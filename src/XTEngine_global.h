#ifndef XTENGINE_GLOBAL_H
#define XTENGINE_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(XTENGINE_LIBRARY)
#  define XTENGINE_EXPORT Q_DECL_EXPORT
#else
#  define XTENGINE_EXPORT Q_DECL_IMPORT
#endif

#endif // XTENGINE_GLOBAL_H
