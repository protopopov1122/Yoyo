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

import java.util.*;
import java.lang.reflect.*;

public class YJIArray extends YJIValue
{
  private final YJI yji;
  public YJIArray(YJI yji, Object a)
   {
     super(Type.Array);
     this.yji = yji;
     this.arr = a;
   }
   public YJIArray(YJI yji, YJIValue[] arr)
   {
     super(Type.Array);
     this.yji = yji;
     this.arr = wrap(arr).getArray();
   }

   public Object getArray()
   {
     return this.arr;
   }
   public YJIValue getYJI(int i)
   {
     return yji.wrap(get(i));
   }
   public void setYJI(int i, YJIValue v)
   {
     set(i, yji.unwrap(v));
   }
   public Object get(int i)
   {
     Class<?> ctype = arr.getClass().getComponentType();
     if (ctype.equals(int.class))
       return ((int[])arr)[i];
     else if (ctype.equals(byte.class))
       return ((byte[])arr)[i];
     else if (ctype.equals(short.class))
       return ((short[])arr)[i];
     else if (ctype.equals(long.class))
       return ((long[])arr)[i];
     else if (ctype.equals(char.class))
       return ((char[])arr)[i];
     else if (ctype.equals(float.class))
       return ((float[])arr)[i];
     else if (ctype.equals(double.class))
       return ((double[])arr)[i];
     else
       return ((Object[])arr)[i];
   }
   public void set(int i, Object v)
   {
     Class<?> ctype = arr.getClass().getComponentType();
     Class<?> vtype = v.getClass();
     if (YJIUtil.compareTypes(ctype, vtype))
     {
       if (ctype.equals(int.class))
       {
         int[] iarr = (int[]) arr;
         iarr[i] = (int) v;
       }
       else if (ctype.equals(byte.class))
       {
         byte[] iarr = (byte[]) arr;
         iarr[i] = (byte) v;
       }
       else if (ctype.equals(short.class))
       {
         short[] iarr = (short[]) arr;
         iarr[i] = (short) v;
       }
       else if (ctype.equals(long.class))
       {
         long[] iarr = (long[]) arr;
         iarr[i] = (long) v;
       }
       else if (ctype.equals(char.class))
       {
         char[] iarr = (char[]) arr;
         iarr[i] = (char) v;
       }
       else if (ctype.equals(float.class))
       {
         float[] iarr = (float[]) arr;
         iarr[i] = (float) v;
       }
       else if (ctype.equals(double.class))
       {
         double[] iarr = (double[]) arr;
         iarr[i] = (double) v;
       }
       else
       {
         Object[] oarr = (Object[]) arr;
         oarr[i] = v;
       }
     }
   }
   public int size()
   {
     Class<?> ctype = arr.getClass().getComponentType();
     if (ctype.equals(int.class))
       return ((int[])arr).length;
     else if (ctype.equals(byte.class))
       return ((byte[])arr).length;
     else if (ctype.equals(short.class))
       return ((short[])arr).length;
     else if (ctype.equals(long.class))
       return ((long[])arr).length;
     else if (ctype.equals(char.class))
       return ((char[])arr).length;
     else if (ctype.equals(float.class))
       return ((float[])arr).length;
     else if (ctype.equals(double.class))
       return ((double[])arr).length;
     else
       return ((Object[])arr).length;
   }
   public Object getValue()
   {
     return this.arr;
   }
   public YJIArray wrap(YJIValue[] a)
   {
     final int BYTE_CL = 0;
     final int SHORT_CL = 1;
     final int CHAR_CL = 2;
     final int INT_CL = 3;
     final int LONG_CL = 4;
     final int FLOAT_CL = 5;
     final int DOUBLE_CL = 6;
     final int BOOL_CL = 7;
     final int OBJECT_CL = 8;
     Map<Class<?>, Integer> levels = new HashMap<Class<?>, Integer>() {
       public static final long serialVersionUID = 42L;
       {
       put(byte.class, 0);
       put(Byte.class, 0);
       put(short.class, 1);
       put(Short.class, 1);
       put(Character.class, 2);
       put(char.class, 2);
       put(Integer.class, 3);
       put(int.class, 3);
       put(Long.class, 4);
       put(int.class, 4);
       put(Float.class, 5);
       put(float.class, 5);
       put(Double.class, 6);
       put(double.class, 6);
       put(Boolean.class, BOOL_CL);
       put(boolean.class, BOOL_CL);
     }};

     int ccl = 0;

     Object[] arr = new Object[a.length];
     for (int i=0;i<arr.length;i++)
       arr[i] = yji.unwrap(a[i]);

     for (int i=0;i<arr.length;i++)
     {
       Class<?> cl = arr[i].getClass();
       if (levels.containsKey(cl))
       {
         int c = levels.get(cl);
         if (c>ccl)
           ccl = c;
       }
       else
       {
         ccl = OBJECT_CL;
         break;
       }
     }
     switch (ccl)
     {
       case INT_CL:
       {
         int[] iarr = new int[arr.length];
         for (int i=0;i<arr.length;i++)
           iarr[i] = ((Number) arr[i]).intValue();
         return new YJIArray(yji, (Object) iarr);
       }
       case BYTE_CL:
       {
         byte[] iarr = new byte[arr.length];
         for (int i=0;i<arr.length;i++)
           iarr[i] = ((Number) arr[i]).byteValue();
         return new YJIArray(yji, (Object) iarr);
       }
       case SHORT_CL:
       {
         short[] iarr = new short[arr.length];
         for (int i=0;i<arr.length;i++)
           iarr[i] = ((Number) arr[i]).shortValue();
         return new YJIArray(yji, (Object) iarr);
       }
       case LONG_CL:
       {
         long[] iarr = new long[arr.length];
         for (int i=0;i<arr.length;i++)
           iarr[i] = ((Number) arr[i]).longValue();
         return new YJIArray(yji, (Object) iarr);
       }
       case CHAR_CL:
       {
         char[] iarr = new char[arr.length];
         for (int i=0;i<arr.length;i++)
           iarr[i] = ((Character) arr[i]);
         return new YJIArray(yji, (Object) iarr);
       }
       case BOOL_CL:
       {
         boolean[] iarr = new boolean[arr.length];
         for (int i=0;i<arr.length;i++)
           iarr[i] = (boolean) arr[i];
         return new YJIArray(yji, (Object) iarr);
       }
       case FLOAT_CL:
       {
         float[] iarr = new float[arr.length];
         for (int i=0;i<arr.length;i++)
           iarr[i] = ((Number) arr[i]).floatValue();
         return new YJIArray(yji, (Object) iarr);
       }
       case DOUBLE_CL:
       {
         double[] iarr = new double[arr.length];
         for (int i=0;i<arr.length;i++)
           iarr[i] = ((Number) arr[i]).doubleValue();
         return new YJIArray(yji, (Object) iarr);
       }
       case OBJECT_CL:
       {
         Object[] iarr = new Object[0];
         if (arr.length>0)
         {
           Class<?> cl = arr[0].getClass();
           for (int i=1;i<arr.length;i++)
          {
            if (arr[i]==null)
              continue;
            Class<?> ncl = arr[i].getClass();
            if (ncl.isAssignableFrom(cl))
              cl = ncl;
          }
          iarr = (Object[]) Array.newInstance(cl, arr.length);
         }
         for (int i=0;i<arr.length;i++)
           iarr[i] = arr[i];
         return new YJIArray(yji, (Object) iarr);
       }
     }
     return new YJIArray(yji, (Object) arr);
   }
   @Override
   public String toString()
   {
     return arr.toString();
   }
   private final Object arr;
}
