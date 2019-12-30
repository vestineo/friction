// enve - 2D animations software
// Copyright (C) 2016-2019 Maurycy Liebner

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

#include "texteffect.h"

#include "Boxes/pathboxrenderdata.h"
#include "Boxes/textboxrenderdata.h"
#include "Boxes/textbox.h"

#include "Animators/qpointfanimator.h"

TextEffect::TextEffect() : eEffect("text effect") {
    mInfluence = enve::make_shared<QrealAnimator>(
                1, 0, 1, 0.1, "influence");
    mTarget = enve::make_shared<ComboBoxProperty>(
                "target", QStringList() << "letters" << "words" << "lines");
    mMinInfluence = enve::make_shared<QrealAnimator>(
                0, 0, 1, 0.1, "min influence");

    mDiminishCont = enve::make_shared<StaticComplexAnimator>("diminish");
    mDiminishInfluence = enve::make_shared<QrealAnimator>(
                1, 0, 1, 0.1, "influence");
    mDiminishCenter = enve::make_shared<QrealAnimator>(
                0, -999, 999, 0.1, "center");
    mDiminishRange = enve::make_shared<QrealAnimator>(
                5, -999, 999, 0.1, "range");
    mDiminishSmoothness = enve::make_shared<QrealAnimator>(
                0.5, 0, 1, 0.1, "smoothness");


    mPeriodicCont = enve::make_shared<StaticComplexAnimator>("periodic");
    mPeriodicInfluence = enve::make_shared<QrealAnimator>(
                0, 0, 1, 0.1, "influence");
    mPeriod = enve::make_shared<QrealAnimator>(
                2, 0.5, 999, 0.1, "period");
    mPeriodicSmoothness = enve::make_shared<QrealAnimator>(
                0.5, 0, 1, 0.1, "smoothness");

    mTransform = enve::make_shared<AdvancedTransformAnimator>();
    //mPathEffects = enve::make_shared<PathEffectAnimators>();
    mRasterEffects = enve::make_shared<RasterEffectAnimators>();
    mRasterEffects->ca_setHiddenWhenEmpty(false);

    ca_addChild(mInfluence);
    ca_addChild(mTarget);
    ca_addChild(mMinInfluence);

    ca_addChild(mDiminishCont);
    mDiminishCont->ca_addChild(mDiminishInfluence);
    mDiminishCont->ca_addChild(mDiminishCenter);
    mDiminishCont->ca_addChild(mDiminishRange);
    mDiminishCont->ca_addChild(mDiminishSmoothness);
    mDiminishCont->ca_setGUIProperty(mDiminishInfluence.get());

    ca_addChild(mPeriodicCont);
    mPeriodicCont->ca_addChild(mPeriodicInfluence);
    mPeriodicCont->ca_addChild(mPeriod);
    mPeriodicCont->ca_addChild(mPeriodicSmoothness);
    mPeriodicCont->ca_setGUIProperty(mPeriodicInfluence.get());


    ca_addChild(mTransform);
    //ca_addChild(mPathEffects);
    ca_addChild(mRasterEffects);

    ca_setGUIProperty(mInfluence.get());
}

bool TextEffect::SWT_dropSupport(const QMimeData * const data) {
    return mRasterEffects->SWT_dropSupport(data);
}

bool TextEffect::SWT_drop(const QMimeData * const data) {
    if(mRasterEffects->SWT_dropSupport(data))
        return mRasterEffects->SWT_drop(data);
    return false;
}

void TextEffect::prp_setupTreeViewMenu(PropertyMenu * const menu) {
    eEffect::prp_setupTreeViewMenu(menu);
    const PropertyMenu::PlainSelectedOp<TextEffect> dOp =
    [](TextEffect* const eff) {
        const auto parent = eff->getParent<DynamicComplexAnimatorBase<TextEffect>>();
        parent->removeChild(eff->ref<TextEffect>());
    };
    menu->addPlainAction("Delete Effect(s)", dOp);
}

QMimeData *TextEffect::SWT_createMimeData() {
    return new eMimeData(QList<TextEffect*>() << this);
}

QMatrix TextEffect::getTransform(const qreal relFrame,
                                 const qreal influence,
                                 const QPointF& addPivot) const {
    const auto pivotAnim = mTransform->getPivotAnimator();
    const auto posAnim = mTransform->getPosAnimator();
    const auto rotAnim = mTransform->getRotAnimator();
    const auto scaleAnim = mTransform->getScaleAnimator();
    const qreal xScale = scaleAnim->getEffectiveXValue(relFrame);
    const qreal yScale = scaleAnim->getEffectiveYValue(relFrame);
    const qreal xPivot = pivotAnim->getEffectiveXValue(relFrame) + addPivot.x();
    const qreal yPivot = pivotAnim->getEffectiveYValue(relFrame) + addPivot.y();
    QMatrix transform;
    transform.translate(xPivot + posAnim->getEffectiveXValue(relFrame)*influence,
                        yPivot + posAnim->getEffectiveYValue(relFrame)*influence);
    transform.rotate(rotAnim->getEffectiveValue(relFrame)*influence);
    transform.scale(1 - influence + xScale*influence,
                    1 - influence + yScale*influence);
    transform.translate(-xPivot, -yPivot);
    return transform;
}

void TextEffect::applyToLetter(LetterRenderData * const letterData,
                               const qreal influence) const {
    if(isZero4Dec(influence)) return;
    const qreal relFrame = letterData->fRelFrame;
    const qreal opacity = 1 + influence*(mTransform->getOpacity(relFrame)*0.01 - 1);

    const auto transform = getTransform(relFrame, influence,
                                        letterData->fLetterPos);
    letterData->applyTransform(transform);
    letterData->fOpacity *= opacity;

    mRasterEffects->addEffects(relFrame, letterData, influence);
}

void TextEffect::applyToWord(WordRenderData * const wordData,
                             const qreal influence) const {
    if(isZero4Dec(influence)) return;
    const qreal relFrame = wordData->fRelFrame;
    const qreal opacity = 1 + influence*(mTransform->getOpacity(relFrame)*0.01 - 1);

    const auto transform = getTransform(relFrame, influence,
                                        wordData->fWordPos);
    wordData->applyTransform(transform);
    wordData->fOpacity *= opacity;

    mRasterEffects->addEffects(relFrame, wordData, influence);
}

void TextEffect::applyToLine(LineRenderData * const lineData,
                             const qreal influence) const {
    if(isZero4Dec(influence)) return;
    const qreal relFrame = lineData->fRelFrame;
    const qreal opacity = 1 + influence*(mTransform->getOpacity(relFrame)*0.01 - 1);

    const auto transform = getTransform(relFrame, influence,
                                        lineData->fLinePos);
    lineData->applyTransform(transform);
    lineData->fOpacity *= opacity;

    mRasterEffects->addEffects(relFrame, lineData, influence);
}

QrealSnapshot diminishGuide(const qreal ampl,
                            const qreal center,
                            const qreal range,
                            const qreal smoothness) {
    QrealSnapshot result(ampl, 1, ampl);
    const qreal x0 = center - qAbs(range);
    const qreal x1 = center;
    const qreal x2 = center + qAbs(range);

    const qreal yMax = qAbs(range) < 1 ? qAbs(range) : 1;

    const qreal y02 = range > 0 ? 0 : yMax;
    const qreal y1 = range > 0 ? yMax : 0;

    result.appendKey(x0, y02,
                     x0, y02,
                     x0*(1 - smoothness) + x1*smoothness, y02);
    result.appendKey(x1*(1 - smoothness) + x0*smoothness, y1,
                     x1, y1,
                     x1*(1 - smoothness) + x2*smoothness, y1);
    result.appendKey(x2*(1 - smoothness) + x1*smoothness, y02,
                     x2, y02,
                     x2, y02);

    return result;
}

QrealSnapshot cyclicalGuide(const qreal ampl,
                            const qreal period,
                            const qreal center,
                            const qreal smoothness,
                            const int nTargets) {
    QrealSnapshot result(ampl, 1, ampl);

    const qreal first = center - qCeil(center/period)*period;
    const qreal last = nTargets + 2*period;
    for(qreal x = first ; x < last; x += period) {
        const qreal xm1 = x - 0.5*period;
        const qreal x0 = x;
        const qreal x1 = x + 0.5*period;
        const qreal x2 = x + period;

        result.appendKey(x0*(1 - smoothness) + xm1*smoothness, 0,
                         x0, 0,
                         x0*(1 - smoothness) + x1*smoothness, 0);
        result.appendKey(x1*(1 - smoothness) + x0*smoothness, 1,
                         x1, 1,
                         x1*(1 - smoothness) + x2*smoothness, 1);
    }
    return result;
}

void TextEffect::apply(TextBoxRenderData * const textData) const {
    const qreal relFrame = textData->fRelFrame;
    const qreal minInfl = mMinInfluence->getEffectiveValue(relFrame);
    const qreal ampl = mInfluence->getEffectiveValue(relFrame);
    const qreal period = mPeriod->getEffectiveValue(relFrame);

    const qreal dimInfl = mDiminishInfluence->getEffectiveValue(relFrame);
    const qreal center = mDiminishCenter->getEffectiveValue(relFrame);
    const qreal diminishRange = mDiminishRange->getEffectiveValue(relFrame);
    const qreal dimSmoothness = mDiminishSmoothness->getEffectiveValue(relFrame);

    const qreal perInfl = mPeriodicInfluence->getEffectiveValue(relFrame);
    const qreal perSmoothness = mPeriodicSmoothness->getEffectiveValue(relFrame);
    const auto baseGuide = diminishGuide(ampl, center, diminishRange, dimSmoothness);

    switch(target()) {
    case TextFragmentType::letter: {
        for(const auto& line : textData->fLines) {
            const auto sinGuide = cyclicalGuide(ampl, period, center, perSmoothness,
                                                line->fString.count());

            int i = 0;
            for(const auto& word : line->fWords) {
                for(const auto& letter : word->fLetters) {
                    const qreal baseInfl = baseGuide.getValue(i)*dimInfl + 1 - dimInfl;
                    const qreal sinInfl = sinGuide.getValue(i)*perInfl + 1 - perInfl;
                    const qreal influence = qBound(minInfl, baseInfl*sinInfl, 1.);
                    i++;
                    applyToLetter(letter.get(), influence);
                }
            }
        }
    } break;
    case TextFragmentType::word: {
        for(const auto& line : textData->fLines) {
            const auto sinGuide = cyclicalGuide(ampl, period, center, perSmoothness,
                                                line->fWords.count());
            int i = 0;
            for(const auto& word : line->fWords) {
                const qreal baseInfl = baseGuide.getValue(i)*dimInfl + 1 - dimInfl;
                const qreal sinInfl = sinGuide.getValue(i)*perInfl + 1 - perInfl;
                const qreal influence = qBound(minInfl, baseInfl*sinInfl, 1.);
                i++;
                applyToWord(word.get(), influence);
            }
        }
    } break;
    case TextFragmentType::line: {
        const auto sinGuide = cyclicalGuide(ampl, period, center, perSmoothness,
                                            textData->fLines.count());
        int i = 0;
        for(const auto& line : textData->fLines) {
            const qreal baseInfl = baseGuide.getValue(i)*dimInfl + 1 - dimInfl;
            const qreal sinInfl = sinGuide.getValue(i)*perInfl + 1 - perInfl;
            const qreal influence = qBound(minInfl, baseInfl*sinInfl, 1.);
            i++;
            applyToLine(line.get(), influence);
        }
    } break;
    }
}

TextFragmentType TextEffect::target() const {
    return static_cast<TextFragmentType>(mTarget->getCurrentValue());
}