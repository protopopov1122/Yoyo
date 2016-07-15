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

public class YJIStaticMethod extends YJIValue
{
  public YJIStaticMethod(Class<?> cl, String method)
  {
    super(Type.StaticMethod);
    this.cl = cl;
    this.method = method;
  }
  public Class<?> getObjectClass()
  {
    return this.cl;
  }
  public String getMethod()
  {
    return this.method;
  }
  private final Class<?> cl;
  private final String method;
}
