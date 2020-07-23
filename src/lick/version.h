// This file is part of the Lick project
//
// Lick is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Lick is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Lick.  If not, see <https://www.gnu.org/licenses/>.

#ifndef VERSION_H
#define VERSION_H

#define AUTHOR          "Stephen Illingworth"
#define PROGNAME        "LiCK"
#define VERSION         "v0.2a"
#define COPYRIGHT       "(C) 2001"
#define DATENUM         "23/05/2001"

#ifdef AMIGA
#define PLATFORM_DESC   "(AmigaOS)"
#else
#define PLATFORM_DESC	"(generic)"
#endif

char *VersTag = "$VER: " PROGNAME " " VERSION " (" DATENUM ") " PLATFORM_DESC;

#endif /* VERSION_H */
