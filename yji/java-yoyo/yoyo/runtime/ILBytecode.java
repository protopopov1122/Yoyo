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

public final class ILBytecode
{
  public ILBytecode(YRuntime runtime)
  {
    this.runtime = runtime;
  }
  public YRuntime getRuntime()
  {
    return this.runtime;
  }

  public native ILProcedure newProcedure();
  public native ILProcedure getProcedure(int pid);
  public native ILProcedure[] getProcedures();

  public native int getSymbolId(String sym);
  public native String getSymbolById(int id);
  public native int[] getSymbolIds();

  public native int addIntegerConstant(long cnst);
  public native int addFloatConstant(double cnst);
  public native int addBooleanConstant(boolean cnst);
  public native int addStringConstant(String cnst);
  public native int addNullConstant();
  public native Object getConstant(int cnst);
  public native int[] getConstantIds();

	public native void export(String path);

  private final YRuntime runtime;
}
