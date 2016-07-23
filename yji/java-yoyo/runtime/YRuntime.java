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

import yoyo.yji.*;


public final class YRuntime
{
  public YRuntime(long ptr, String path)
  {
		if (first_load)
			System.load(path);
		first_load = false;
    this.runtime = ptr;
    init();
    this.closed = false;
    this.bytecode = new ILBytecode(this);
    this.yji = new YJI(this);
  }
  public YJI getYJI()
  {
    return this.yji;
  }
  public ILBytecode getBytecode()
  {
    return this.bytecode;
  }
  public void close()
  {
    if (!closed)
    {
      closed = true;
      destroy();
    }
  }

  public Object interpret(int pid)
  {
    return yji.unwrap(_interpret(pid));
  }

  protected native YJIValue _interpret(int pid);

  protected native void init();
  protected native void destroy();

  private boolean closed;
  private ILBytecode bytecode;
  private long runtime;
  private final YJI yji;

	private static boolean first_load = true;
}
