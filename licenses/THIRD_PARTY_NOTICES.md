# Open-source notices

Speech Rate Meter 2 contains or links the open-source components listed
below. The complete license texts are available from the **Open-source
licences** page in the application and are included in the APK/AAB.

The application's own source files are offered under the MIT License. The
distributed executable also incorporates ALGLIB under the GNU GPL, so the
combined executable is distributed under the GNU GPL terms described below.

## Qt 6.12.0

Copyright (C) The Qt Company Ltd. and other contributors.

Qt Quick, Qt Multimedia, and their runtime dependencies are dynamically linked
under the GNU Lesser General Public License version 3. Qt is a registered
trademark of The Qt Company Ltd.

Corresponding Qt source:
https://download.qt.io/official_releases/qt/6.12/6.12.0/submodules/

The Qt shared libraries in the Android package may be replaced with
interface-compatible modified builds. Rebuild the application using the
instructions in `README.md` and a Qt 6.12.0 Android kit built from the source
above. The generated APK can then be signed with your own key and installed on
an Android device. No project term prohibits reverse engineering for debugging
modifications to LGPL-covered Qt libraries.

## ALGLIB 4.06.0

Copyright (C) Sergey Bochkanov and ALGLIB contributors.

The GPL edition of ALGLIB is compiled into the application and is licensed
under GNU GPL version 2 or, at your option, any later version. The complete
corresponding application source, ALGLIB source, CMake files, and Android build
instructions are available at:

https://github.com/zhitko/speech-rate-meter-2

Matching upstream archive:
https://www.alglib.net/translator/re/alglib-4.06.0.cpp.gpl.zip

To rebuild or relink a modified version, follow `README.md`. Preserve the directory
`3rdparty/alglib-cpp/src` when replacing ALGLIB with a compatible modified
version.

## SPTK 4.3

Copyright 2021 SPTK Working Group and contributors.

SPTK is statically linked under the Apache License 2.0. The build uses SPTK
commit `68f4158`. SPTK includes the following components whose notices are
also provided in the application:

- WORLD — BSD 3-Clause-style license; copyright (c) 2010 M. Morise.
- REAPER — Apache License 2.0.
- SWIPE — MIT License; copyright (c) 2009-2013 Kyle Gorman.
- Snack — permissive license; copyright (C) 2000-2005 Jonas Beskow and Kare
  Sjolander.
- ya_getopt — BSD 2-Clause-style license; copyright 2015 Kubo Takehiro.

Source: https://github.com/sp-nitech/SPTK/tree/68f4158

## Font Awesome Free 7.2.0

Copyright (c) 2026 Fonticons, Inc.

The bundled Font Awesome web/desktop font files are licensed under the SIL Open
Font License 1.1. “Font Awesome” is a Reserved Font Name.

Source and licence information: https://fontawesome.com/license/free

## LLVM OpenMP runtime

The Android package dynamically links `libomp.so` from Android NDK
27.2.12479018 (r27c). LLVM OpenMP is licensed under the Apache License 2.0 with
LLVM Exceptions. The complete applicable text is included as
`LLVM-OpenMP-LICENSE.txt`.

Source: https://github.com/llvm/llvm-project/tree/llvmorg-18.1.8/openmp

## Android platform dependencies

Qt's generated Android package can include AndroidX runtime components. Those
components are licensed under the Apache License 2.0; their package-specific
notices and source are available from the Android Open Source Project and
AndroidX:

https://source.android.com/
https://github.com/androidx/androidx

## Source availability

Keep the source repository and the exact version used for every published
binary available for at least as long as that binary is distributed.
Android package id: `by.intoncore.SpeechRateMeter2`.
A release source archive must include the application source, plus the
matching ALGLIB and SPTK source trees or unambiguous access to those exact
sources. Requests concerning source or relinking may also be filed through
the repository's issue tracker.

These notices are informational and do not replace the full license texts.
