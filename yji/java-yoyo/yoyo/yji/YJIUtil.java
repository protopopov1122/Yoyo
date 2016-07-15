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

import java.lang.reflect.*;
import java.util.*;

public class YJIUtil
{
  public static Object callStaticMethod(YJI yji, Class<?> obj, String name, Object... args) throws Exception
  {
    Class<?>[] clargs = new Class<?>[args.length];
    for (int i=0;i<args.length;i++)
    {
      if (args[i]!=null)
        clargs[i] = args[i].getClass();
      else
        clargs[i] = Object.class;
    }
    Method method = getMethod(obj.getMethods(), name, clargs);
    if (method!=null)
    {
      args = castArguments(yji, method.getParameterTypes(), args);
      method.setAccessible(true);
      return method.invoke(null, args);
    }
    return null;
  }
  public static Object callConstructor(YJI yji, Class<?> cl, Object... args) throws Exception
  {
    Class<?>[] clargs = new Class<?>[args.length];
    for (int i=0;i<args.length;i++)
    {
      if (args[i]!=null)
        clargs[i] = args[i].getClass();
      else
        clargs[i] = Object.class;
    }
    Constructor<?> method = getConstructor(cl.getConstructors(), clargs);
    if (method!=null)
    {
      args = castArguments(yji, method.getParameterTypes(), args);
      method.setAccessible(true);
      try {
        return method.newInstance(args);
      } catch (Exception e) {
        e.printStackTrace();
      }
    }
    return null;
  }
  public static Object callMethod(YJI yji, Object obj, String name, Object[] args) throws Exception
  {
    Class<?>[] clargs = new Class<?>[args.length];
    for (int i=0;i<args.length;i++)
    {
      if (args[i]!=null)
        clargs[i] = args[i].getClass();
      else
        clargs[i] = Object.class;
    }
    Class<?> objClass = obj.getClass();
    Method method = getMethod(objClass.getMethods(), name, clargs);
    if (method!=null)
    {
      args = castArguments(yji, method.getParameterTypes(), args);
      method.setAccessible(true);
      return method.invoke(obj, args);
    }
    return null;
  }
public static Method getMethod(Method[] meths, String name, Class<?>[] args)
{
  for (Method method : meths)
  {
    if (!method.getName().equals(name))
      continue;
    if (compareTypes(method.getParameterTypes(), args))
      return method;
  }
  return null;
}
public static Constructor<?> getConstructor(Constructor<?>[] meths, Class<?>[] args)
{
  for (Constructor<?> method : meths)
  {
    if (compareTypes(method.getParameterTypes(), args))
      return method;
  }
  return null;
}
public static Object[] castArguments(YJI yji, Class<?>[] types, Object[] arr)
{
  Object[] out = new Object[arr.length];
  for (int i=0;i<arr.length;i++)
    out[i] = castArgument(yji, types[i], arr[i]);
  return out;
}
public static Object castArgument(YJI yji, Class<?> type, Object arg)
{
  Class<?> argType = arg.getClass();
  if (wrappers.containsKey(type))
    type = wrappers.get(type);
  if (wrappers.containsKey(argType))
    argType = wrappers.get(argType);
    if (type.equals(argType)||isAssignableFrom(type, argType))
      return argType.cast(arg);
  if (type.isArray()&&argType.isArray())
  {
    YJIArray arr = new YJIArray(yji, arg);
    Class<?> comType = type.getComponentType();
    if (comType.equals(int.class))
    {
      int[] out = new int[arr.size()];
      for (int i=0;i<out.length;i++)
        out[i] = ((Number) arr.get(i)).intValue();
      return out;
    }
    else if (comType.equals(byte.class))
    {
      byte[] out = new byte[arr.size()];
      for (int i=0;i<out.length;i++)
        out[i] = ((Number) arr.get(i)).byteValue();
      return out;
    }
    else if (comType.equals(short.class))
    {
      short[] out = new short[arr.size()];
      for (int i=0;i<out.length;i++)
        out[i] = ((Number) arr.get(i)).shortValue();
      return out;
    }
    else if (comType.equals(long.class))
    {
      long[] out = new long[arr.size()];
      for (int i=0;i<out.length;i++)
        out[i] = ((Number) arr.get(i)).longValue();
      return out;
    }
    else if (comType.equals(float.class))
    {
      float[] out = new float[arr.size()];
      for (int i=0;i<out.length;i++)
        out[i] = ((Number) arr.get(i)).floatValue();
      return out;
    }
    else if (comType.equals(double.class))
    {
      double[] out = new double[arr.size()];
      for (int i=0;i<out.length;i++)
        out[i] = ((Number) arr.get(i)).doubleValue();
      return out;
    }
    else if (comType.equals(char.class))
    {
      char[] out = new  char[arr.size()];
      for (int i=0;i<out.length;i++)
        out[i] = ((Character) arr.get(i));
      return out;
    }
    else if (comType.equals(boolean.class))
    {
      boolean[] out = new boolean[arr.size()];
      for (int i=0;i<out.length;i++)
        out[i] = ((Boolean) arr.get(i));
      return out;
    }
    else
    {
      Object[] out = new Object[arr.size()];
      for (int i=0;i<out.length;i++)
        out[i] = arr.get(i);
      return out;
    }
  }
  return null;
}
public static boolean compareTypes(Class<?>[] a1, Class<?>[] a2)
{
  if (a1.length!=a2.length)
    return false;
  for (int i=0;i<a1.length;i++)
    if (!compareTypes(a1[i], a2[i]))
      return false;
  return true;
}
public static boolean compareTypes(Class<?> c1, Class<?> c2)
{
  if (wrappers.containsKey(c1))
    c1 = wrappers.get(c1);
  if (wrappers.containsKey(c2))
    c2 = wrappers.get(c2);

  if (c1.equals(c2)||isAssignableFrom(c1, c2))
    return true;
  if (c1.isArray()&&c2.isArray())
    return compareTypes(c1.getComponentType(), c2.getComponentType());
  return false;
}
private static boolean isAssignableFrom(Class<?> c1, Class<?> c2)
{
  if (c1.equals(c2)||c1.isAssignableFrom(c2))
    return true;
  if (wrappers2.containsKey(c1))
    c1 = wrappers2.get(c1);
  if (wrappers2.containsKey(c2))
    c2 = wrappers2.get(c2);
  if (c1.equals(byte.class))
    return c2.equals(byte.class);
  if (c1.equals(short.class))
    return c2.equals(byte.class)||c2.equals(short.class);
  if (c1.equals(int.class))
    return c2.equals(byte.class)||c2.equals(short.class)||c2.equals(int.class);
  return false;
}
private static Map<Class<?>, Class<?>> wrappers = new HashMap<Class<?>, Class<?>>() {
  public static final long serialVersionUID = 42L;
  {
    put(boolean.class, Boolean.class);
    put(char.class, Character.class);
    put(int.class, Integer.class);
    put(long.class, Long.class);
    put(byte.class, Byte.class);
    put(short.class, Short.class);
    put(float.class, Float.class);
    put(double.class, Double.class);
}};
private static Map<Class<?>, Class<?>> wrappers2 = new HashMap<Class<?>, Class<?>>() {
  public static final long serialVersionUID = 42L;
  {
    put(Boolean.class, boolean.class);
    put(Character.class, char.class);
    put(Integer.class, int.class);
    put(Long.class, long.class);
    put(Byte.class, byte.class);
    put(Short.class, short.class);
    put(Float.class, float.class);
    put(Double.class, double.class);
}};

}
