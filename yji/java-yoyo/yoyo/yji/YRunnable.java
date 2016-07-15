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

package yoyo.yji;

import yoyo.runtime.*;

public final class YRunnable
{
  public Object invoke(Object... args)
  {
    YJIValue[] yjiargs = new YJIValue[args.length];
    for (int i=0;i<args.length;i++)
      yjiargs[i] = yji.wrap(args[i]);
    return yji.unwrap(_invoke(yjiargs));
  }
  protected native YJIValue _invoke(YJIValue[] args);


  public void close()
  {
    if (!destroyed)
    {
      destroyed = true;
      destroy();
    }
  }
  @Override
  public void finalize()
  {
    close();
  }
  protected native void init();
  protected native void destroy();
  protected YRunnable(YRuntime runtime, long defenv, long ptr)
  {
    this.runtime = runtime;
    this.yji = new YJI(this.runtime);
    this.defenv = defenv;
    this.ptr = ptr;
    this.destroyed = false;
    init();
  }

  private final YJI yji;
  private final YRuntime runtime;
  private final long defenv;
  private final long ptr;
  private boolean destroyed;
}
