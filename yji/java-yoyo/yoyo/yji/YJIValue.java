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

public abstract class YJIValue
{
  public YJIValue(Type t)
  {
    this.type = t;
  }
  public boolean isInt()
  {
    return type==Type.Int;
  }
  public boolean isFloat()
  {
    return type==Type.Float;
  }
  public boolean isString()
  {
    return type==Type.String;
  }
  public boolean isBoolean()
  {
    return type==Type.Boolean;
  }
  public boolean isObject()
  {
    return type==Type.Object;
  }
  public boolean isMethod()
  {
    return type==Type.Method;
  }
  public boolean isArray()
  {
    return type==Type.Array;
  }
  public boolean isClass()
  {
    return type==Type.Class;
  }
  public boolean isStaticMethod()
  {
    return type==Type.StaticMethod;
  }
  public static enum Type
  {
    Int, Float, String, Boolean, Object, Method, Array, Class, StaticMethod;
  }
  private final Type type;
}
