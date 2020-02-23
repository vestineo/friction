// enve - 2D animations software
// Copyright (C) 2016-2020 Maurycy Liebner

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef ZIPFILELOADER_H
#define ZIPFILELOADER_H

#include <quazip/quazipfile.h>

#include "exceptions.h"

class ZipFileLoader {
public:
    ZipFileLoader();
    ~ZipFileLoader() { mZip.close(); }

    void setZipPath(const QString& path);

    using Processor = std::function<void(QIODevice* const src)>;
    void process(const QString& file, const bool text,
                 const Processor& func);
private:
    QuaZip mZip;
    QuaZipFile mFile;
};

#endif // ZIPFILELOADER_H
