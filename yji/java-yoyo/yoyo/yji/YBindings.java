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
import yoyo.yji.*;

public final class YBindings
{
  public byte getByte(String key)
  {
    Object o = get(key);
    if (o instanceof Number)
      return ((Number)o).byteValue();
    return 0;
  }
  public short getShort(String key)
  {
    Object o = get(key);
    if (o instanceof Number)
      return ((Number)o).shortValue();
    return 0;
  }
  public int getInt(String key)
  {
    Object o = get(key);
    if (o instanceof Number)
      return ((Number)o).intValue();
    return 0;
  }
  public long getLong(String key)
  {
    Object o = get(key);
    if (o instanceof Number)
      return ((Number)o).longValue();
    return 0;
  }
  public String getString(String key)
  {
    Object o = get(key);
    if (o instanceof String)
      return (String) o;
    return o.toString();
  }
  public float getFloat(String key)
  {
    Object o = get(key);
    if (o instanceof Number)
      return ((Number)o).floatValue();
    return 0f;
  }
  public double getDouble(String key)
  {
      Object o = get(key);
      if (o instanceof Number)
        return ((Number)o).doubleValue();
      return 0;
  }
  public YJIArray getArray(String key)
  {
      Object o = get(key);
      if (o.getClass().isArray())
        return new YJIArray(yji, o);
      return null;
  }
  public Object get(String key)
  {
    return yji.unwrap(_get(runtime.getBytecode().getSymbolId(key)));
  }
  public void put(String key, boolean n, Object o)
  {
    _put(runtime.getBytecode().getSymbolId(key), n, yji.wrap(o));
  }
  public void put(String key, Object o)
  {
    _put(runtime.getBytecode().getSymbolId(key), false, yji.wrap(o));
  }
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
  protected native YJIValue _get(int key);
  protected native void _put(int key, boolean n, YJIValue v);
  protected native void init();
  protected native void destroy();

  protected YBindings(YRuntime runtime, long defenv, long ptr)
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
