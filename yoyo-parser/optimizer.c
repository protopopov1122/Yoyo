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

#include "parser.h"
#include "math.h"

YNode* fold_constants(YNode* nd) {
	if (nd->type==UnaryN) {
		YUnaryNode* un = (YUnaryNode*) nd;
		if (un->argument->type==ConstantN) {
			yconstant_t cnst = ((YConstantNode*) un->argument)->id;
			switch (un->operation) {
				case Negate: {
					if (cnst.type==Int64Constant) {
						nd->free(nd);
						yconstant_t newc = {.type = Int64Constant};
						newc.value.i64 = -cnst.value.i64;
						return newConstantNode(newc);
					} else if (cnst.type==Int64Constant) {
						nd->free(nd);
						yconstant_t newc = {.type = Int64Constant};
						newc.value.fp64 = -cnst.value.fp64;
						return newConstantNode(newc);
					}
				}
				break;
				case Not: {
					if (cnst.type==Int64Constant) {
						nd->free(nd);
						yconstant_t newc = {.type = Int64Constant};
						newc.value.i64 = ~cnst.value.i64;
						return newConstantNode(newc);
					}
				}
				break;
				default:
				break;
			}
		}
	} else 	if (nd->type==BinaryN) {
		YBinaryNode* bin = (YBinaryNode*) nd;
		if (bin->left->type==ConstantN&&
				bin->right->type==ConstantN) {
			yconstant_t left = ((YConstantNode*) bin->left)->id;
			yconstant_t right = ((YConstantNode*) bin->right)->id;
			bool integer = left.type == Int64Constant &&
											right.type == Int64Constant;
			bool fpoint = (left.type == Int64Constant && right.type == Fp64Constant) ||
											(left.type == Fp64Constant && right.type == Int64Constant) ||
											(left.type == Fp64Constant && right.type == Fp64Constant);
			#define getDouble(c) (c.type == Fp64Constant ? c.value.fp64 :\
														(c.type == Int64Constant ? (double) c.value.i64 : 0))
			switch (bin->operation) {
				case Add: {
					if (integer) {
						nd->free(nd);
						yconstant_t newc = {.type = Int64Constant};
						newc.value.i64 = left.value.i64 + right.value.i64;
						return newConstantNode(newc);
					} else if (fpoint) {
						double d1 = getDouble(left);
						double d2 = getDouble(right);
						nd->free(nd);
						yconstant_t newc = {.type = Fp64Constant};
						newc.value.fp64 = d1 + d2;
						return newConstantNode(newc);
					}

				}
				break;
				case Subtract: {
					if (integer) {
						nd->free(nd);
						yconstant_t newc = {.type = Int64Constant};
						newc.value.i64 = left.value.i64 - right.value.i64;
						return newConstantNode(newc);
					} else if (fpoint) {
						double d1 = getDouble(left);
						double d2 = getDouble(right);
						nd->free(nd);
						yconstant_t newc = {.type = Fp64Constant};
						newc.value.fp64 = d1 - d2;
						return newConstantNode(newc);
					}

				}
				break;
				case Multiply: {
					if (integer) {
						nd->free(nd);
						yconstant_t newc = {.type = Int64Constant};
						newc.value.i64 = left.value.i64 * right.value.i64;
						return newConstantNode(newc);
					} else if (fpoint) {
						double d1 = getDouble(left);
						double d2 = getDouble(right);
						nd->free(nd);
						yconstant_t newc = {.type = Fp64Constant};
						newc.value.fp64 = d1 * d2;
						return newConstantNode(newc);
					}
				}
				break;
				case Divide: {
					if (integer && right.value.i64 != 0) {
						nd->free(nd);
						yconstant_t newc = {.type = Int64Constant};
						newc.value.i64 = left.value.i64 / right.value.i64;
						return newConstantNode(newc);
					} else if (fpoint) {
						double d1 = getDouble(left);
						double d2 = getDouble(right);
						nd->free(nd);
						yconstant_t newc = {.type = Fp64Constant};
						newc.value.fp64 = d1 / d2;
						return newConstantNode(newc);
					}

				}
				break;
				case Modulo: {
					if (integer && right.value.i64 != 0) {
						nd->free(nd);
						yconstant_t newc = {.type = Int64Constant};
						newc.value.i64 = left.value.i64 % right.value.i64;
						return newConstantNode(newc);
					}

				}
				break;
				case Power: {
					if (integer) {
						nd->free(nd);
						yconstant_t newc = {.type = Int64Constant};
						newc.value.i64 = pow(left.value.i64, right.value.i64);
						return newConstantNode(newc);
					} else if (fpoint) {
						double d1 = getDouble(left);
						double d2 = getDouble(right);
						nd->free(nd);
						yconstant_t newc = {.type = Fp64Constant};
						newc.value.fp64 = pow(d1, d2);
						return newConstantNode(newc);
					}

				}
				break;
				case ShiftLeft: {
					if (integer) {
						nd->free(nd);
						yconstant_t newc = {.type = Int64Constant};
						newc.value.i64 = left.value.i64 << right.value.i64;
						return newConstantNode(newc);
					}

				}
				break;
				case ShiftRight: {
					if (integer) {
						nd->free(nd);
						yconstant_t newc = {.type = Int64Constant};
						newc.value.i64 = left.value.i64 >> right.value.i64;
						return newConstantNode(newc);
					}

				}
				break;
				case And: {
					if (integer) {
						nd->free(nd);
						yconstant_t newc = {.type = Int64Constant};
						newc.value.i64 = left.value.i64 & right.value.i64;
						return newConstantNode(newc);
					}

				}
				break;
				case Or: {
					if (integer) {
						nd->free(nd);
						yconstant_t newc = {.type = Int64Constant};
						newc.value.i64 = left.value.i64 | right.value.i64;
						return newConstantNode(newc);
					}

				}
				break;
				case Xor: {
					if (integer) {
						nd->free(nd);
						yconstant_t newc = {.type = Int64Constant};
						newc.value.i64 = left.value.i64 ^ right.value.i64;
						return newConstantNode(newc);
					}

				}
				break;
				default:
				break;
			}
		}
	}
	return nd;
}

YNode* optimize_node(YNode* nd) {
	return fold_constants(nd);
}
