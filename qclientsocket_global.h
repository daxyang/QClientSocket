#ifndef QCLIENTSOCKET_GLOBAL_H
#define QCLIENTSOCKET_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(QCLIENTSOCKET_LIBRARY)
#  define QCLIENTSOCKETSHARED_EXPORT Q_DECL_EXPORT
#else
#  define QCLIENTSOCKETSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // QCLIENTSOCKET_GLOBAL_H