/***********************************************************************[version.hpp]
Copyright(c) 2020, Muhammad Osama - Anton Wijs,
Technische Universiteit Eindhoven (TU/e).

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
**********************************************************************************/

namespace ParaFROST {

	const char* version();
	const char* signature();
	const char* compiler();
	const char* compilemode();
	const char* osystem();
	const char* date();

}
#define VERSION "3.2.5"
#define OSYSTEM "darwin ronan-bras-macbook-pro 21.6.0 arm64"
#define DATE "Fri Dec 13 13:48:57 PST 2024"
#define COMPILER "Apple clang version 14.0.0 (clang-1400.0.29.102)"
