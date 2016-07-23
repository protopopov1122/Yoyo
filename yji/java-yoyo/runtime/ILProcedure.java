/*
 * Copyright (C) 2016  Jevgenijs Protopopovs <protopopov1122@yandex.ru>
 */
/*This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.*/

package yoyo.runtime;

public final class ILProcedure
{
  public ILProcedure()
  {

  }

  public native int getIdentifier();
  public native void setRegisterCount(int regc);
  public native int getRegisterCount();
  public native byte[] getCode();
  public native int getCodeLength();
  public native void appendCode(byte opcode, int arg1, int arg2, int arg3);
  public native void appendCode(byte... code);
  public native void addCodeTableEntry(int line, int charPos, int offset, int length, int file);
  public native void addLabel(int id, int value);
  public native int getLabel(int id);
  public native int[] getLabels();
  public native void free();

  private long proc;
}
