#pragma once

#include <QtCore/qglobal.h>

#if defined(QTMODELLIBRARY_LIBRARY)
#define QTMODELLIBRARY_EXPORT Q_DECL_EXPORT
#else
#define QTMODELLIBRARY_EXPORT Q_DECL_IMPORT
#endif
