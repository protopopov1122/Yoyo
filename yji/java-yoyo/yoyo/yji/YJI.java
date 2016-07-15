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

import java.io.*;
import java.lang.reflect.*;

import yoyo.runtime.*;

public class YJI
{
  public YJI(YRuntime runtime)
  {
    this.runtime = runtime;
  }
  public YJIValue call(YJIMethod meth, YJIValue[] args) throws Exception
  {
    Object[] oargs = new Object[args.length];
    for (int i=0;i<args.length;i++)
      oargs[i] = unwrap(args[i]);
    Object o = YJIUtil.callMethod(this, meth.getObject(), meth.getMethod(), oargs);
    return wrap(o);
  }
  public YJIValue callStatic(YJIStaticMethod ysm, YJIValue[] args) throws Exception
  {
    Object[] oargs = new Object[args.length];
    for (int i=0;i<args.length;i++)
      oargs[i] = unwrap(args[i]);
    return wrap(YJIUtil.callStaticMethod(this, ysm.getObjectClass(), ysm.getMethod(), oargs));
  }
  public YJIValue get(Object o, int key)
  {
    Class<?> cl = o.getClass();
    String fieldn = runtime.getBytecode().getSymbolById(key);
    try {
      for (Field field : cl.getDeclaredFields())
      {
        if (field.getName().equals(fieldn))
        {
          Object val = field.get(o);
          return wrap(val);
        }
      }
      for (Method method : cl.getMethods())
      {
        if (method.getName().equals(fieldn))
        {
          return new YJIMethod(o, fieldn);
        }
      }
    } catch (Exception e) {}
    return null;
  }
  public boolean contains(Object o, int key)
  {
    if (o==null)
      return false;
    Class<?> cl = o.getClass();
    String fieldn = runtime.getBytecode().getSymbolById(key);
    try {
      for (Field field : cl.getDeclaredFields())
      {
        if (field.getName().equals(fieldn))
        {
          return true;
        }
      }
      for (Method method : cl.getMethods())
      {
        if (method.getName().equals(fieldn))
        {
          return true;
        }
      }
    } catch (Exception e) {}
    return false;
  }
  public YJIValue loadStatic(Class<?> cl, int key) throws Exception
  {
  String fieldn = runtime.getBytecode().getSymbolById(key);
    try {
      for (Field field : cl.getFields())
      {
        if (field.getName().equals(fieldn)&&Modifier.isStatic(field.getModifiers()))
        {
          Object rawValue = field.get(null);
          return wrap(rawValue);
        }
      }
      for (Method meth : cl.getMethods())
        if (meth.getName().equals(fieldn)&&Modifier.isStatic(meth.getModifiers()))
          return new YJIStaticMethod(cl, fieldn);
    } catch (Exception e) {
    }
    return null;
  }
  public boolean containsStatic(Class<?> cl, int key) throws Exception
  {
  String fieldn = runtime.getBytecode().getSymbolById(key);
    try {
      for (Field field : cl.getFields())
      {
        if (field.getName().equals(fieldn)&&Modifier.isStatic(field.getModifiers()))
        {
          return true;
        }
      }
      for (Method meth : cl.getMethods())
        if (meth.getName().equals(fieldn)&&Modifier.isStatic(meth.getModifiers()))
          return true;
    } catch (Exception e) {
    }
    return false;
  }
  public void set(Object o, int key, YJIValue val)
  {
    Object value = unwrap(val);
    Class<?> cl = o.getClass();
    String fieldn = runtime.getBytecode().getSymbolById(key);
    try {
      for (Field field : cl.getDeclaredFields())
      {
        if (field.getName().equals(fieldn))
        {
          field.set(o, value);
        }
      }
    } catch (Exception e) {e.printStackTrace();}
}
  public YJIValue wrap(Object o)
  {
    if (o==null)
      return null;
    if (o instanceof Byte || o instanceof Short
    || o instanceof Integer || o instanceof Long)
      return new YJIInteger(((Number)o).longValue());
    if (o instanceof Float || o instanceof Double)
          return new YJIFloat(((Number)o).doubleValue());
    if (o instanceof Boolean)
      return new YJIBoolean((Boolean) o);
    if (o instanceof String)
      return new YJIString(runtime.getBytecode().getSymbolId((String) o));
    if (o instanceof Class)
      return new YJIClass(((Class<?>)o));
    Class<?> ocl = o.getClass();
    if (ocl.isArray())
      return new YJIArray(this, o);
    if (o instanceof YJIValue)
      return (YJIValue) o;
    return new YJIObject(o);
  }
  public Object unwrap(YJIValue val)
  {
    if (val==null)
      return null;
    if (val.isInt())
    {
      long l = ((YJIInteger)val).getValue();
      if (l>=Byte.MIN_VALUE&&l<=Byte.MAX_VALUE)
        return (byte) l;
      if (l>=Short.MIN_VALUE&&l<=Short.MAX_VALUE)
        return (short) l;
      if (l>=Integer.MIN_VALUE&&l<=Integer.MAX_VALUE)
        return (int) l;
      return l;
    }
    else if (val.isFloat())
    {
      double d = ((YJIFloat)val).getValue();
      if (d>=Float.MIN_VALUE&&d<=Float.MAX_VALUE)
        return (float) d;
      return d;
    }
    else if (val.isBoolean())
      return ((YJIBoolean)val).getValue();
    else if (val.isString())
      return runtime.getBytecode().getSymbolById(((YJIString)val).getId());
    else if (val.isObject())
      return ((YJIObject)val).getValue();
    else if (val.isArray())
      return ((YJIArray)val).getArray();
    else if (val.isClass())
      return ((YJIClass)val).getValue();
    return null;
  }
  public YRuntime getRuntime()
  {
    return runtime;
  }
  private final YRuntime runtime;
}
