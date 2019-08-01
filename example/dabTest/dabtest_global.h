#ifndef DABTEST_GLOBAL_H
#define DABTEST_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(DabTest_LIBRARY)
#  define DABTESTSHARED_EXPORT Q_DECL_EXPORT
#else
#  define DABTESTSHARED_EXPORT Q_DECL_IMPORT
#endif

#include "enveCore/RasterEffects/customrastereffect.h"
#include "enveCore/RasterEffects/rastereffectcaller.h"

extern "C" {

DABTESTSHARED_EXPORT
    qsptr<CustomRasterEffect> createNewestVersionEffect();

DABTESTSHARED_EXPORT
    qsptr<CustomRasterEffect> createEffect(
        const CustomIdentifier &identifier);

DABTESTSHARED_EXPORT
    QString effectName();

DABTESTSHARED_EXPORT
    CustomIdentifier effectIdentifier();

DABTESTSHARED_EXPORT
    bool supports(const CustomIdentifier &identifier);
}
#endif // DABTEST_GLOBAL_H
