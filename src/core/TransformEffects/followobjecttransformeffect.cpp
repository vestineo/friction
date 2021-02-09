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

#include "followobjecttransformeffect.h"

#include "Boxes/boundingbox.h"
#include "Animators/qrealanimator.h"
#include "Animators/transformanimator.h"

FollowObjectTransformEffect::FollowObjectTransformEffect() :
    TransformEffect("follow object", TransformEffectType::followObject) {
    mTarget = enve::make_shared<BoxTargetProperty>("target");
    mPosInfluence = enve::make_shared<QPointFAnimator>(
                        QPointF{1., 1.}, QPointF{-10., -10.},
                        QPointF{10., 10.}, QPointF{0.01, 0.01},
                        "pos influence");
    mScaleInfluence = enve::make_shared<QPointFAnimator>(
                        QPointF{1., 1.}, QPointF{-10., -10.},
                        QPointF{10., 10.}, QPointF{0.01, 0.01},
                        "scale influence");
    mRotInfluence = enve::make_shared<QrealAnimator>(
                        1, -10, 10, 0.01, "rot influence");

    connect(mTarget.get(), &BoxTargetProperty::targetSet,
            this, [this](BoundingBox* const newTarget) {
        auto& conn = mTargetConn.assign(newTarget);
        if(newTarget) {
            const auto parent = getFirstAncestor<BoundingBox>();
            const auto parentTransform = parent->getTransformAnimator();
            const auto targetTransform = newTarget->getTransformAnimator();
            conn << connect(targetTransform, &Property::prp_absFrameRangeChanged,
                            this, [parentTransform](const FrameRange& range,
                            const bool clip) {
                parentTransform->prp_afterChangedAbsRange(range, clip);
            });
            conn << connect(targetTransform,
                            &AdvancedTransformAnimator::totalTransformChanged,
                            this, [parentTransform](const UpdateReason reason) {
                parentTransform->prp_afterChangedCurrent(reason);
            });
        }
    });

    ca_addChild(mTarget);
    ca_addChild(mPosInfluence);
    ca_addChild(mScaleInfluence);
    ca_addChild(mRotInfluence);
}


void FollowObjectTransformEffect::applyEffect(
        const qreal relFrame,
        qreal& pivotX, qreal& pivotY,
        qreal& posX, qreal& posY,
        qreal& rot,
        qreal& scaleX, qreal& scaleY,
        qreal& shearX, qreal& shearY,
        BoundingBox* const parent) {
    Q_UNUSED(pivotX);
    Q_UNUSED(pivotY);
    Q_UNUSED(shearX);
    Q_UNUSED(shearY);

    if(!parent) return;
    const auto target = mTarget->getTarget();
    if(!target) return;
    const auto targetTransformAnimator = target->getTransformAnimator();

    const qreal absFrame = prp_relFrameToAbsFrameF(relFrame);
    const qreal targetRelFrame = target->prp_absFrameToRelFrameF(absFrame);

    const auto parentTransform = parent->getInheritedTransformAtFrame(relFrame);
    const auto targetTransform = target->getTotalTransformAtFrame(targetRelFrame);

    const auto transform = targetTransform*parentTransform.inverted();
    const auto relPivot = target->getPivotRelPos(targetRelFrame);
    const auto p1 = relPivot*transform;

    const auto rotAnim = targetTransformAnimator->getRotAnimator();
    const qreal targetRot = rotAnim->getEffectiveValue(targetRelFrame);

    const auto scaleAnim = targetTransformAnimator->getScaleAnimator();
    const qreal xScale = scaleAnim->getEffectiveXValue(targetRelFrame);
    const qreal yScale = scaleAnim->getEffectiveYValue(targetRelFrame);

    const qreal posXInfl = mPosInfluence->getEffectiveXValue(relFrame);
    const qreal posYInfl = mPosInfluence->getEffectiveYValue(relFrame);
    const qreal scaleXInfl = mScaleInfluence->getEffectiveXValue(relFrame);
    const qreal scaleYInfl = mScaleInfluence->getEffectiveYValue(relFrame);
    const qreal rotInfl = mRotInfluence->getEffectiveValue(relFrame);

    posX += p1.x()*posXInfl;
    posY += p1.y()*posYInfl;

    scaleX *= 1 + (xScale - 1)*scaleXInfl;
    scaleY *= 1 + (yScale - 1)*scaleYInfl;

    rot += targetRot*rotInfl;
}
