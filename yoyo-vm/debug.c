/*
 * Copyright (C) 2016  Jevgenijs Protopopovs <protopopov1122@yandex.ru>
 */
/*This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License version 3 as published by
 the Free Software Foundation.


 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.*/

#include "yoyo-runtime.h"
#include "interpreter.h"

/* File contains methods that use Yoyo debugger calls
 * to create simply command-line debugger. Debugger
 * stores breakpoints and being called after execution start&end,
 * after each instruction. If current state fits to breakpoint or
 * because of certain flags debugger must pause execution than
 * being called command-line. Command line accepts user input
 * and parses it to change debugger state, continue execution
 * or execute some code.
 * */

typedef struct DbgBreakpoint {
	int32_t fileid;
	uint32_t line;
	wchar_t* condition;
} DbgBreakpoint;

typedef struct DbgPager {
	FILE* out_stream;
	FILE* in_stream;
	uint16_t page_size;
	uint16_t string;
	bool enabled;
} DbgPager;

void Pager_print(DbgPager* pager) {
	if (!pager->enabled)
		return;
	if (pager->string<pager->page_size) {
		pager->string++;
		return;
	}
	pager->string = 0;
	const char* mess =  "Press <ENTER> to continue";
	fprintf(pager->out_stream, "%s", mess);
	fflush(pager->out_stream);
	free(readLine(pager->in_stream));
#ifdef OS_UNIX 
	printf("\033[1A");
	printf("%c[2K", 27);
#endif
}

void Pager_session(DbgPager* pager) {
	pager->string = 0;
}

typedef struct DefaultDebugger {
	YDebug debug;

	DbgBreakpoint* breakpoints;
	size_t bp_count;
	ILBytecode* bytecode;

	bool pager;
	uint16_t page_size;
} DefaultDebugger;

#define LINE_FLAG 0
#define NEXT_FLAG 1
#define STEP_FLAG 2

void DefaultDebugger_cli(YDebug* debug, YThread* th) {
	((ExecutionFrame*) th->frame)->debug_flags &= ~(1 << NEXT_FLAG);
	((ExecutionFrame*) th->frame)->debug_flags &= ~(1 << STEP_FLAG);
	DefaultDebugger* dbg = (DefaultDebugger*) debug;
	bool work = true;
	DbgPager pager = {.out_stream = th->runtime->env->out_stream, .in_stream = th->runtime->env->in_stream,
		.page_size = dbg->page_size, .enabled = dbg->pager,
		.string = 0};
#define CMD(l, cmd) if (wcscmp(l, cmd)==0)
	while (work) {
		ILProcedure* proc =
				dbg->bytecode->procedures[((ExecutionFrame*) th->frame)->proc->id];
		CodeTableEntry* e = proc->getCodeTableEntry(proc,
				((ExecutionFrame*) th->frame)->pc);
		if (e != NULL) {
			wchar_t* fileid = dbg->bytecode->getSymbolById(dbg->bytecode,
					e->file);
			fprintf(th->runtime->env->out_stream, "%ls:%"PRId32, fileid,
					e->line);
		} else
			fprintf(th->runtime->env->out_stream, "?");
		fprintf(th->runtime->env->out_stream, "> ");
		fflush(th->runtime->env->out_stream);
		wchar_t* line = readLine(stdin);

		wchar_t* origLine = calloc(1, sizeof(wchar_t) * (wcslen(line) + 1));
		wcscpy(origLine, line);
		uint32_t symcount = 0;
		for (size_t i = 0; i < wcslen(line); i++)
			if (!iswspace(line[i]))
				symcount++;
		if (wcslen(line) == 0 || symcount == 0) {
			free(line);
			continue;
		}
		wchar_t** argv = NULL;
		size_t argc = 0;
		WCSTOK_STATE(state);
		for (wchar_t* arg = WCSTOK(line, L" \t", &state); arg != NULL; arg =
				WCSTOK(NULL, L" \t", &state)) {
			argc++;
			argv = realloc(argv, sizeof(wchar_t*) * argc);
			argv[argc - 1] = arg;
		}

		Pager_session(&pager);

		CMD(argv[0], L"run") {
			work = false;
		} else CMD(argv[0], L"next") {
			((ExecutionFrame*) th->frame)->debug_flags |= 1 << NEXT_FLAG;
			if (e != NULL) {
				((ExecutionFrame*) th->frame)->debug_ptr = (void*) e;
			} else
				((ExecutionFrame*) th->frame)->debug_ptr = NULL;
			work = false;
		} else CMD(argv[0], L"step") {
			((ExecutionFrame*) th->frame)->debug_flags |= 1 << NEXT_FLAG;
			((ExecutionFrame*) th->frame)->debug_flags |= 1 << STEP_FLAG;
			if (e != NULL) {
				((ExecutionFrame*) th->frame)->debug_ptr = (void*) e;
			} else
				((ExecutionFrame*) th->frame)->debug_ptr = NULL;
			work = false;
		} else CMD(argv[0], L"break?") {
			if (argc > 2) {
				uint32_t bpid = wcstoul(argv[1], NULL, 0);
				if (bpid < dbg->bp_count) {
					StringBuilder* sb = newStringBuilder(L"");
					for (size_t i = 2; i < argc; i++) {
						sb->append(sb, argv[i]);
						sb->append(sb, L" ");
					}
					sb->string[wcslen(sb->string) - 1] = L'\0';
					free(dbg->breakpoints[bpid].condition);
					dbg->breakpoints[bpid].condition = sb->string;
					free(sb);
				}
			}
		} else CMD(argv[0], L"break") {
			wchar_t* file = NULL;
			wchar_t* wline = NULL;
			if (argc > 2) {
				file = argv[1];
				wline = argv[2];
			} else if (argc > 1) {
				wline = argv[1];
				if (((ExecutionFrame*) th->frame) != NULL) {
					ExecutionFrame* frame = ((ExecutionFrame*) th->frame);
					CodeTableEntry* e = proc->getCodeTableEntry(proc,
							frame->pc);
					if (e != NULL)
						file = dbg->bytecode->getSymbolById(dbg->bytecode,
								e->file);
				}
			}
			int32_t fid = dbg->bytecode->getSymbolId(dbg->bytecode, file);
			uint32_t line = wcstoul(wline, NULL, 0);
			dbg->bp_count++;
			dbg->breakpoints = realloc(dbg->breakpoints,
					sizeof(DbgBreakpoint) * dbg->bp_count);
			dbg->breakpoints[dbg->bp_count - 1].fileid = fid;
			dbg->breakpoints[dbg->bp_count - 1].line = line;
			dbg->breakpoints[dbg->bp_count - 1].condition = NULL;
		} else CMD(argv[0], L"ls") {
			if (argc > 1) {
				wchar_t* req = argv[1];
				CMD(req, L"files") {
					wchar_t** files = th->runtime->env->getLoadedFiles(
							th->runtime->env);
					if (files != NULL) {
						size_t i = 0;
						while (files[i] != NULL)
							fprintf(th->runtime->env->out_stream, "\t%ls\n",
									files[i++]);
					} else
						fprintf(th->runtime->env->out_stream,
								"Information not available\n");
				} else CMD(req, L"line") {
					if (e != NULL) {
						FILE* file = th->runtime->env->getFile(th->runtime->env,
								dbg->bytecode->getSymbolById(dbg->bytecode,
										e->file));
						uint32_t first = e->line;
						uint32_t last = e->line;
						uint32_t currentLine = e->line;
						wint_t wch;
						uint32_t line = 1;
						if (argc > 2)
							first -= wcstoul(argv[2], NULL, 0);
						if (argc > 3)
							last += wcstoul(argv[3], NULL, 0);
						Pager_print(&pager);
						fprintf(th->runtime->env->out_stream,
							"%"PRIu32":\t", first == 0 ? first + 1 : first);
						while ((wch = fgetwc(file)) != WEOF) {
							if (line >= first
									&& !(line == last && wch == L'\n'))
								fprintf(th->runtime->env->out_stream, "%lc",
										wch);
							if (wch == L'\n') {
								line++;
								if (line > last)
									break;
								if (line >= first) {
									Pager_print(&pager);
									if (line != first)
										fprintf(th->runtime->env->out_stream,
											"%"PRIu32":", line);
									if (line == currentLine)
										fprintf(th->runtime->env->out_stream,
											">>");
									fprintf(th->runtime->env->out_stream, "\t");
								}
							}
						}
						fclose(file);
						fprintf(th->runtime->env->out_stream, "\n");
					} else
						fprintf(th->runtime->env->out_stream,
								"Information not available\n");
				} else CMD(req, L"file") {
					FILE* file = NULL;
					uint32_t currentLine = -1;
					if (argc > 2)
						file = th->runtime->env->getFile(th->runtime->env,
								argv[2]);
					if (file == NULL && ((ExecutionFrame*) th->frame) != NULL) {
						SourceIdentifier sid = th->frame->get_source_id(th->frame);	
						if (sid.file != -1) {
							wchar_t* wname = dbg->bytecode->getSymbolById(
									dbg->bytecode, sid.file);
							file = th->runtime->env->getFile(th->runtime->env,
									wname);
							currentLine = sid.line;
							fprintf(th->runtime->env->out_stream,
									"Listing file '%ls':\n", wname);
						}
					}
					if (file != NULL) {
						wint_t wch;
						uint32_t line = 1;
						uint32_t first = 1;
						uint32_t last = UINT32_MAX;
						if (argc > 3)
							first = wcstoul(argv[3], NULL, 0);
						if (argc > 4)
							last = wcstoul(argv[4], NULL, 0);
						if (first == 1) {
							Pager_print(&pager);
							fprintf(th->runtime->env->out_stream,
									"%"PRIu32":\t", first);
						}
						while ((wch = fgetwc(file)) != WEOF) {
							if (line >= first
									&& !(line == last && wch == L'\n'))
								fprintf(th->runtime->env->out_stream, "%lc",
										wch);
							if (wch == L'\n') {
								line++;
								Pager_print(&pager);	
								if (line > last)
									break;
								if (line >= first) {
									fprintf(th->runtime->env->out_stream,
											"%"PRIu32":", line);
									if (line == currentLine)
										fprintf(th->runtime->env->out_stream, ">>");
									fprintf(th->runtime->env->out_stream, "\t");
								}
							}
						}
						fclose(file);
						fprintf(th->runtime->env->out_stream, "\n");
					} else if (argc > 2)
						fprintf(th->runtime->env->out_stream,
								"File '%ls' not found\n", argv[2]);
					else
						fprintf(th->runtime->env->out_stream,
								"File not found\n");
				} else CMD(req, L"breaks") {
					for (size_t i = 0; i < dbg->bp_count; i++) {
						DbgBreakpoint* bp = &dbg->breakpoints[i];
						Pager_print(&pager);	
						wchar_t* fileid = dbg->bytecode->getSymbolById(
								dbg->bytecode, bp->fileid);
						if (bp->condition == NULL)
							fprintf(th->runtime->env->out_stream,
									"#"SIZE_T" at %ls:%"PRIu32"\n", i, fileid,
									bp->line);
						else
							fprintf(th->runtime->env->out_stream,
									"#"SIZE_T" at %ls:%"PRIu32" (%ls)\n", i,
									fileid, bp->line, bp->condition);
					}
				} else CMD(req, L"procs") {
					ILBytecode* bc = dbg->bytecode;
					size_t pcount = 0;
					for (size_t i = 0; i < bc->procedure_count; i++)
						if (bc->procedures[i] != NULL)
							pcount++;
					Pager_print(&pager);
					fprintf(th->runtime->env->out_stream,
							"Procedure count: "SIZE_T"\n", pcount);
					for (size_t i = 0; i < dbg->bytecode->procedure_count;
							i++) {
						ILProcedure* proc = dbg->bytecode->procedures[i];
						if (proc == NULL)
							continue;
						Pager_print(&pager);
						if (((ExecutionFrame*) th->frame)->proc->id == proc->id)
							fprintf(th->runtime->env->out_stream, "*");
						fprintf(th->runtime->env->out_stream,
								"\tProcedure #%"PRIu32"(regc=%"PRIu32"; code="SIZE_T" bytes)\n",
								proc->id, proc->regc, proc->code_length);
					}
				} else CMD(req, L"proc") {
					int32_t pid =
							((ExecutionFrame*) th->frame) != NULL ?
									((ExecutionFrame*) th->frame)->proc->id :
									-1;
					ssize_t pc = ((ExecutionFrame*) th->frame) != NULL ?
									((ExecutionFrame*) th->frame)->pc :
									-1;
					if (argc == 3)
						pid = wcstoul(argv[2], NULL, 0);
					ILBytecode* bc = dbg->bytecode;
					if (pid != -1&& pid < bc->procedure_count &&
					bc->procedures[pid]!=NULL) {
						ILProcedure* proc = bc->procedures[pid];
						Pager_print(&pager);
						fprintf(th->runtime->env->out_stream,
								"Procedure #%"PRIu32":\n", proc->id);;
						Pager_print(&pager);
						fprintf(th->runtime->env->out_stream,
								"Register count = %"PRIu32"\n", proc->regc);
						Pager_print(&pager);
						fprintf(th->runtime->env->out_stream,
								"Code length = "SIZE_T" bytes\n",
								proc->code_length);
						Pager_print(&pager);
						fprintf(th->runtime->env->out_stream,
								"Label table("SIZE_T" entries):\n",
								proc->labels.length);
						for (size_t i = 0; i < proc->labels.length; i++) {
							Pager_print(&pager);
							fprintf(th->runtime->env->out_stream,
									"\t%"PRId32"\t%"PRIu32"\n",
									proc->labels.table[i].id,
									proc->labels.table[i].value);
						}
						Pager_print(&pager);
						fprintf(th->runtime->env->out_stream, "\nCode:\n");
						for (size_t i = 0; i < proc->code_length; i += 13) {
							Pager_print(&pager);
							for (size_t j = 0; j < proc->labels.length; j++)
								if (proc->labels.table[j].value == i)
									fprintf(th->runtime->env->out_stream,
											"%"PRId32":",
											proc->labels.table[j].id);
							if (i==pc)
								fprintf(th->runtime->env->out_stream, ">>");
							wchar_t* mnem = L"Unknown";
							for (size_t j = 0; j < OPCODE_COUNT; j++)
								if (Mnemonics[j].opcode == proc->code[i]) {
									mnem = Mnemonics[j].mnemonic;
									break;
								}
							int32_t* iargs = (int32_t*) &(proc->code[i + 1]);
							fprintf(th->runtime->env->out_stream,
									"\t"SIZE_T"\t%ls: %"PRId32"; %"PRId32"; %"PRId32"\n",
									i, mnem, iargs[0], iargs[1], iargs[2]);
						}
					}
				} else CMD(req, L"bytecode") {
					ILBytecode* bc = dbg->bytecode;
					size_t pcount = 0;
					for (size_t i = 0; i < bc->procedure_count; i++)
						if (bc->procedures[i] != NULL)
							pcount++;
					fprintf(th->runtime->env->out_stream,
							"Procedure count: "SIZE_T"\n", pcount);
					fprintf(th->runtime->env->out_stream,
							"Constant pool size: %"PRIu32"\n",
							bc->constants.size);
					fprintf(th->runtime->env->out_stream,
							"Symbol table length: "SIZE_T"\n",
							th->runtime->symbols.size);
				} else CMD(req, L"symbols") {
					fprintf(th->runtime->env->out_stream,
							"Symbol table length: "SIZE_T"\n",
							th->runtime->symbols.size);
					for (size_t i = 0; i < th->runtime->symbols.size; i++)
						fprintf(th->runtime->env->out_stream,
								"\t%"PRId32"\t\"%ls\"\n",
								th->runtime->symbols.map[i].id,
								th->runtime->symbols.map[i].symbol);
				} else CMD(req, L"constants") {
					ILBytecode* bc = dbg->bytecode;
					Pager_print(&pager);
					fprintf(th->runtime->env->out_stream,
							"Constant pool size: %"PRIu32"\n",
							bc->constants.size);
					for (size_t i = 0; i < bc->constants.size; i++) {
						Pager_print(&pager);
						fprintf(th->runtime->env->out_stream, "\t%"PRId32"\t",
								bc->constants.pool[i]->id);
						Constant* cnst = bc->constants.pool[i];
						switch (cnst->type) {
						case IntegerC:
							fprintf(th->runtime->env->out_stream, "int %"PRId64,
									cnst->value.i64);
							break;
						case FloatC:
							fprintf(th->runtime->env->out_stream, "float %lf",
									cnst->value.fp64);
							break;
						case StringC:
							fprintf(th->runtime->env->out_stream,
									"string \"%ls\"",
									bc->getSymbolById(bc,
											cnst->value.string_id));
							break;
						case BooleanC:
							fprintf(th->runtime->env->out_stream, "boolean %s",
									cnst->value.boolean ?
											"true" : "false");
							break;
						case NullC:
							fprintf(th->runtime->env->out_stream, "null");
							break;
						}
						fprintf(th->runtime->env->out_stream, "\n");
					}
				} else CMD(req, L"runtime") {
					Pager_print(&pager);
					if (th->runtime->newObject == newTreeObject)
						fprintf(th->runtime->env->out_stream, "Object: AVL tree\n");
					else
						fprintf(th->runtime->env->out_stream, "Object: hash table\n");
					Pager_print(&pager);
					fprintf(th->runtime->env->out_stream, "Integer pool size: "SIZE_T"\n"
								"Integer cache size: "SIZE_T"\n",
								th->runtime->Constants.IntPoolSize,
								th->runtime->Constants.IntCacheSize);
					Pager_print(&pager);
					fprintf(th->runtime->env->out_stream, "Thread count: "SIZE_T"\n", th->runtime->thread_count);
					for (YThread* t = th->runtime->threads; t!=NULL; t = t->prev) {
						Pager_print(&pager);
						fprintf(th->runtime->env->out_stream, "\tThread #%"PRIu32, t->id);
						if (t->frame != NULL) {
							SourceIdentifier sid = t->frame->get_source_id(t->frame);
							if (sid.file!=-1)
								fprintf(th->runtime->env->out_stream, " at %ls(%"PRIu32":%"PRIu32")", getSymbolById(&th->runtime->symbols, sid.file),
									sid.line, sid.charPosition);
						}
						fprintf(th->runtime->env->out_stream, "\n");
					}
				} else
					fprintf(th->runtime->env->out_stream,
							"Unknown argument '%ls'. Use 'help' command.\n",
							req);
			} else fprintf(th->runtime->env->out_stream, "Command prints runtime information. Available arguments:\n"
										"\tfiles - list loaded files\n"
										"\tfile - list file. Format: ls file [name]\n"
										"\tline - list current line/lines before and after it. Format: ls line [before] [after]\n"
										"\tbytecode - list common information about bytecode\n"
										"\tprocs - procedure list\n"
										"\tproc -  list procedure. Format: ls proc [id]\n"
										"\tsymbols - list symbol and identifiers\n"
										"\tconstants - list constants\n"
										"\tbreaks - list breakpoints\n"
										"\truntime - list common runtime information\n");
		} else CMD(argv[0], L"rm") {
			if (argc > 1) {
				wchar_t* req = argv[1];
				CMD(req, L"break") {
					if (argc == 3) {
						wchar_t* wid = argv[2];
						size_t id = wcstoul(wid, NULL, 0);
						if (id < dbg->bp_count) {
							free(dbg->breakpoints[id].condition);
							DbgBreakpoint* newBp = malloc(
									sizeof(DbgBreakpoint) * dbg->bp_count - 1);
							memcpy(newBp, dbg->breakpoints,
									sizeof(DbgBreakpoint) * id);
							memcpy(&newBp[id], &dbg->breakpoints[id + 1],
									sizeof(DbgBreakpoint)
											* (dbg->bp_count - 1 - id));
							dbg->bp_count--;
							free(dbg->breakpoints);
							dbg->breakpoints = newBp;
						}
					}
				} else
					fprintf(th->runtime->env->out_stream,
							"Unknown argument '%ls'. Use 'help' command.\n",
							req);
			}
		} else CMD(argv[0], L"eval") {
			if (argc > 1) {
				wchar_t* code = &origLine[wcslen(L"eval") + 1];
				th->runtime->state = RuntimeRunning;
				YValue* val = th->runtime->env->execute(th->runtime->env,
						th->runtime, string_input_stream(code), L"<eval>",
						(YObject*) ((ExecutionFrame*) th->frame)->regs[0]);
				if (th->exception != NULL) {
					wchar_t* wstr = toString(th->exception, th);
					fprintf(th->runtime->env->out_stream, "%ls\n", wstr);
					free(wstr);
					th->exception = NULL;
				}
				if (val->type != &th->runtime->NullType) {
					wchar_t* wstr = toString(val, th);
					fprintf(th->runtime->env->out_stream, "%ls\n", wstr);
					free(wstr);
				}
				th->runtime->state = RuntimePaused;
			}
		} else CMD(argv[0], L"trace") {
			YThread* t = th;
			if (argc > 1) {
				uint32_t tid = (uint32_t) wcstoul(argv[1], NULL, 0);
				for (t = th->runtime->threads; t != NULL; t = t->prev)
					if (t->id == tid) {
						break;
					}
				if (t==NULL) {
					fprintf(th->runtime->env->out_stream, "Thread #%"PRIu32" not found\n", tid);
					continue;
				}
			}
			LocalFrame* frame = t->frame;
			while (frame != NULL) {
				SourceIdentifier sid = frame->get_source_id(frame);
				if (sid.file != -1)
					fprintf(th->runtime->env->out_stream,
							"\t%ls(%"PRIu32":%"PRIu32")\n",
							getSymbolById(&th->runtime->symbols,
									sid.file), sid.line, sid.charPosition);
				frame = frame->prev;
			}
		} else CMD(argv[0], L"pager") {
			if (argc>1) {
				CMD(argv[1], L"enable") {
					dbg->pager = true;
					pager.enabled = true;
				} else CMD(argv[1], L"disable") {
					dbg->pager = false;
					pager.enabled = false;
				} else {
					uint16_t ps = (uint16_t) wcstoul(argv[1], NULL, 0);
					if (ps>0) {
						dbg->page_size = ps;
						pager.page_size = ps;
					}
				}
			} else {
				fprintf(th->runtime->env->out_stream,
					"Current state: %s(%"PRIu16" lines per page)\n",
					pager.enabled ? "enabled" : "disabled", pager.page_size);
				fprintf(th->runtime->env->out_stream,
					"Use:\n\t'pager enable' - to enable pager\n\t'pager disable' - to disable pager\n\t'pager [number]' - set page size\n");
			}
		} else CMD(argv[0], L"quit") {
			exit(0);
		} else CMD(argv[0], L"nodebug") {
			debug->mode = NoDebug;
			work = false;
		} else CMD(argv[0], L"help")
			fprintf(th->runtime->env->out_stream, "Yoyo debugger.\n"
						"Available commands:\n"
						"\tbreak - add breakpoint. Format: break [filename] line\n"
						"\tbreak? - add condition to existing breakpoint. Format: break? id condition\n"
						"\trun - continue execution\n"
						"\tnext - execute next line\n"
						"\tstep - execute next line, step into function on call\n"
						"\tnodebug - stop debugging, continue execution\n"
						"\tquit - stop execution, quit debugger\n"
						"\ttrace - print thread backtrace. Format: trace [thread id]\n"
						"\teval - execute code in current scope\n"
						"\trm - remove breakpoint. Format: rm break breakpoint_id\n"
						"\tls - list information. Type 'ls' for manual\n"
						"\tpager - control built-in pager. Type 'pager' for manual and info\n");
		else
			fprintf(th->runtime->env->out_stream,
					"Unknown command '%ls'. Use 'help' command.\n", argv[0]);

		fflush(th->runtime->env->out_stream);
		free(argv);
		free(origLine);
		free(line);
	}
#undef CMD
}

/* There stored reaction on debugging events*/
void DefaultDebugger_onload(YDebug* debug, void* ptr, YThread* th) {
	fprintf(th->runtime->env->out_stream, "Default Yoyo Debugger started\n");
}

void DefaultDebugger_interpret_start(YDebug* debug, void* ptr, YThread* th) {
	fprintf(th->runtime->env->out_stream, "Thread #%"PRIu32" started\n",
			th->id);
	DefaultDebugger_cli(debug, th);
}

void DefaultDebugger_interpret_end(YDebug* debug, void* ptr, YThread* th) {
	if (th->exception!=NULL) {
		wchar_t* wstr = toString(th->exception, th);
		fprintf(th->runtime->env->out_stream, "%ls\n", wstr);
		free(wstr);
		YValue* e = th->exception;
		if (e->type == &th->runtime->ObjectType) {
				YObject* obj = (YObject*) e;
				if (OBJECT_HAS(obj, L"trace", th)) {
					YValue* trace = OBJECT_GET(obj, L"trace", th);
					wchar_t* wcs = toString(trace, th);
					fprintf(th->runtime->env->out_stream, "%ls\n", wcs);
					free(wcs);
				}
			}
	}
	fprintf(th->runtime->env->out_stream, "Thread #%"PRIu32" finished\n",
			th->id);
	DefaultDebugger_cli(debug, th);
}

/* This procedure called before execution of any instruction.
 * It translates current interpreter state to source
 * code file and line. If there is breakpoint on this line
 * or command on last line was 'step' or 'next' then cli being called.
 * For each line debugger calls cli only before first instruction.  */
void DefaultDebugger_instruction(YDebug* debug, void* ptr, YThread* th) {
	DefaultDebugger* dbg = (DefaultDebugger*) debug;
	YBreakpoint* bp = (YBreakpoint*) ptr;
	ILProcedure* proc = dbg->bytecode->procedures[bp->procid];
	CodeTableEntry* e = proc->getCodeTableEntry(proc, bp->pc);
	if (e != NULL) {
		for (size_t i = 0; i < dbg->bp_count; i++) {
			DbgBreakpoint* dbbp = &dbg->breakpoints[i];
			if (dbbp->fileid == e->file && dbbp->line == e->line) {
				if ((((ExecutionFrame*) th->frame)->debug_flags
						& (1 << LINE_FLAG)) == 0
						|| ((ExecutionFrame*) th->frame)->debug_field
								!= dbbp->line) {
					/* On this line is breakpoint and it is
					 * line first instruction*/
					bool exec = true;
					if (dbbp->condition != NULL) {
						/* Checks if breakpoint has condition and
						 * executes this condition */
						th->runtime->state = RuntimeRunning;
						YValue* val =
								th->runtime->env->execute(th->runtime->env,
										th->runtime,
										string_input_stream(dbbp->condition),
										dbbp->condition,
										(YObject*) ((ExecutionFrame*) th->frame)->regs[0]);
						th->exception = NULL;
						if (val->type == &th->runtime->BooleanType) {
							exec = ((YBoolean*) val)->value;
						}
						th->runtime->state = RuntimePaused;
					}
					if (exec) {
						wchar_t* sym = dbg->bytecode->getSymbolById(
								dbg->bytecode, dbbp->fileid);
						fprintf(th->runtime->env->out_stream,
								"Reached breakpoint #"SIZE_T" at %ls:%"PRIu32"\n",
								i, sym, dbbp->line);
						DefaultDebugger_cli(debug, th);
					}
				}
				((ExecutionFrame*) th->frame)->debug_flags |= 1 << LINE_FLAG;
				((ExecutionFrame*) th->frame)->debug_field = dbbp->line;
				return;
			}
		}
		if ((((ExecutionFrame*) th->frame)->debug_flags & (1 << STEP_FLAG))
				== 1 << STEP_FLAG) {
			CodeTableEntry* last_e =
					(CodeTableEntry*) ((ExecutionFrame*) th->frame)->debug_ptr;
			if (last_e != NULL && last_e->line != e->line) {
				((ExecutionFrame*) th->frame)->debug_flags &= ~(1 << STEP_FLAG);
			}
		}
		if ((((ExecutionFrame*) th->frame)->debug_flags & (1 << NEXT_FLAG))
				== 1 << NEXT_FLAG) {
			CodeTableEntry* last_e =
					(CodeTableEntry*) ((ExecutionFrame*) th->frame)->debug_ptr;
			if (last_e == NULL)
				DefaultDebugger_cli(debug, th);
			if (last_e != NULL && last_e->line != e->line) {
				((ExecutionFrame*) th->frame)->debug_flags &= ~(1 << NEXT_FLAG);
				DefaultDebugger_cli(debug, th);
			}
		}
	}
	((ExecutionFrame*) th->frame)->debug_flags &= ~(1 << LINE_FLAG);
	((ExecutionFrame*) th->frame)->debug_field = -1;
}
void DefaultDebugger_enter_function(YDebug* debug, void* ptr, YThread* th) {
	if (ptr == NULL)
		return;
	ExecutionFrame* last_frame = (ExecutionFrame*) ptr;
	if ((last_frame->debug_flags & (1 << STEP_FLAG)) == 1 << STEP_FLAG) {
		CodeTableEntry* last_e = (CodeTableEntry*) last_frame->debug_ptr;
		if (last_e == NULL) {
			((ExecutionFrame*) th->frame)->debug_flags &= ~(1 << STEP_FLAG);
			fprintf(th->runtime->env->out_stream,
					"Stepped into function at %ls:%"PRIu32"\n",
					getSymbolById(&th->runtime->symbols, last_e->file),
					last_e->line);
		}
		DefaultDebugger_cli(debug, th);
	}
}

void DefaultDebugger_free(YDebug* debug) {
	DefaultDebugger* dbg = (DefaultDebugger*) debug;
	for (size_t i = 0; i < dbg->bp_count; i++) {
		free(dbg->breakpoints[i].condition);
	}
	free(dbg->breakpoints);
	free(dbg);
}

YDebug* newDefaultDebugger(ILBytecode* bc) {
	DefaultDebugger* dbg = calloc(1, sizeof(DefaultDebugger));

	dbg->bp_count = 0;
	dbg->breakpoints = NULL;
	dbg->bytecode = bc;
	dbg->pager = true;
	dbg->page_size = 25;

	YDebug* debug = (YDebug*) dbg;
	debug->mode = Debug;

	debug->onload = DefaultDebugger_onload;
	debug->interpret_start = DefaultDebugger_interpret_start;
	debug->interpret_end = DefaultDebugger_interpret_end;
	debug->instruction = DefaultDebugger_instruction;
	debug->enter_function = DefaultDebugger_enter_function;
	debug->free = DefaultDebugger_free;

	return debug;
}
