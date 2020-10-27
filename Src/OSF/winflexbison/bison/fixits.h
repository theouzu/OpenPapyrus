/* Support for fixing grammar files.

   Copyright (C) 2019-2020 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef FIXITS_H_
#define FIXITS_H_ 1
	void fixits_register(Location const * loc, char const* update); /* Declare a fix to apply.  */
	void fixits_run(); /* Apply the fixits: update the source file.  */
	bool fixits_empty(); /* Whether there are no fixits. */
	void fixits_free(); /* Free the registered fixits.  */
#endif /* !FIXITS_H_ */