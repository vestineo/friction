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

#include "graphkey.h"
#include "qrealpoint.h"
#include "animator.h"

GraphKey::GraphKey(const int frame,
                   Animator * const parentAnimator) :
    Key(parentAnimator) {
    mRelFrame = frame;
    setC0FrameVar(mRelFrame - 5);
    setC1FrameVar(mRelFrame + 5);
    mC0Point = enve::make_shared<QrealPoint>(QrealPointType::c0Pt, this, 4);
    mKeyPoint = enve::make_shared<QrealPoint>(QrealPointType::keyPt, this, 6);
    mC1Point = enve::make_shared<QrealPoint>(QrealPointType::c1Pt, this, 4);
}

GraphKey::GraphKey(Animator * const parentAnimator) :
    GraphKey(0, parentAnimator) {}


void GraphKey::changeFrameAndValueBy(const QPointF &frameValueChange) {
    const int newFrame = qRound(frameValueChange.x() + mSavedRelFrame);
    const bool frameChanged = newFrame != mRelFrame;
    if(!frameChanged) return;
    if(mParentAnimator) {
        mParentAnimator->anim_moveKeyToRelFrame(this, newFrame);
    } else {
        setRelFrame(newFrame);
    }
}

void GraphKey::startFrameTransform() {
    Key::startFrameTransform();
    mC0Clamped.saveValue();
    mC1Clamped.saveValue();

    mSavedC0Enabled = mC0Enabled;
    mSavedC1Enabled = mC1Enabled;
}

void GraphKey::finishFrameTransform() {
    //    if(!mParentAnimator) return;
    //    mParentAnimator->addUndoRedo(
    //                new ChangeKeyFrameUndoRedo(mSavedRelFrame,
    //                                           mRelFrame, this));
}

void GraphKey::cancelFrameTransform() {
    Key::cancelFrameTransform();
    setC0FrameVar(mC0Clamped.getRawSavedXValue());
    setC1FrameVar(mC1Clamped.getRawSavedXValue());
    setC0ValueVar(mC0Clamped.getRawSavedYValue());
    setC1ValueVar(mC1Clamped.getRawSavedYValue());
    setC0Enabled(mSavedC0Enabled);
    setC1Enabled(mSavedC1Enabled);
}


void GraphKey::writeKey(eWriteStream& dst) {
    Key::writeKey(dst);

    dst << mC0Enabled;
    dst.write(&mC0Clamped, sizeof(ClampedPoint));

    dst << mC1Enabled;
    dst.write(&mC1Clamped, sizeof(ClampedPoint));
}

void GraphKey::readKey(eReadStream& src) {
    Key::readKey(src);

    if(src.evFileVersion() >= 3) {
        src >> mC0Enabled;
        src.read(&mC0Clamped, sizeof(ClampedPoint));

        src >> mC1Enabled;
        src.read(&mC1Clamped, sizeof(ClampedPoint));
    }
}

void GraphKey::scaleFrameAndUpdateParentAnimator(
        const int relativeToFrame,
        const qreal scaleFactor,
        const bool useSavedFrame) {
    const int thisRelFrame = useSavedFrame ? mSavedRelFrame : mRelFrame;
    const qreal c0RelFrame = useSavedFrame ?
                mC0Clamped.getRawSavedXValue() :
                mC0Clamped.getRawXValue();
    const qreal c1RelFrame = useSavedFrame ?
                mC1Clamped.getRawSavedXValue() :
                mC1Clamped.getRawXValue();

    const int relPivot =
            mParentAnimator->prp_absFrameToRelFrame(relativeToFrame);

    const int newFrame = qRound(relPivot + (thisRelFrame - relPivot)*scaleFactor);
    if(newFrame != mRelFrame) {
        incFrameAndUpdateParentAnimator(newFrame - mRelFrame);
    } else {
        mParentAnimator->anim_updateAfterChangedKey(this);
    }


    const qreal actualScale = qreal(newFrame - relPivot)/
                                   (thisRelFrame - relPivot);
    const bool switchCtrls = actualScale < 0;
    setC0Enabled(switchCtrls ? mSavedC1Enabled :
                                          mSavedC0Enabled);
    setC1Enabled(switchCtrls ? mSavedC0Enabled :
                                        mSavedC1Enabled);

    const qreal newc0 = relPivot + (c0RelFrame - relPivot)*actualScale;
    const qreal newc1 = relPivot + (c1RelFrame - relPivot)*actualScale;
    setC0FrameVar(switchCtrls ? newc1 : newc0);
    setC1FrameVar(switchCtrls ? newc0 : newc1);

    if(switchCtrls) {
        setC0ValueVar(mC1Clamped.getRawSavedYValue());
        setC1ValueVar(mC0Clamped.getRawSavedYValue());
    } else {
        setC0ValueVar(mC0Clamped.getRawSavedYValue());
        setC1ValueVar(mC1Clamped.getRawSavedYValue());
    }
}

QrealPoint *GraphKey::mousePress(const qreal frame,
                                 const qreal value,
                                 const qreal pixelsPerFrame,
                                 const qreal pixelsPerValue) {
    if(isSelected()) {
        if(getC0Enabled() && hasPrevKey()) {
            const bool isC0Near = mC0Point->isNear(
                        frame, value, pixelsPerFrame, pixelsPerValue);
            if(isC0Near) return mC0Point.get();
        }
        if(getC1Enabled() && hasNextKey()) {
            const bool isC1Near = mC1Point->isNear(
                        frame, value, pixelsPerFrame, pixelsPerValue);
            if(isC1Near) return mC1Point.get();
        }
    }
    if(mKeyPoint->isNear(frame, value, pixelsPerFrame, pixelsPerValue)) {
        return mKeyPoint.get();
    }
    return nullptr;
}

void GraphKey::updateCtrlFromCtrl(const QrealPointType type) {
    if(mCtrlsMode == CtrlsMode::corner) return;
    QPointF fromPt;
    QPointF toPt;
    QrealPoint *targetPt;
    if(type == QrealPointType::c1Pt) {
        toPt = QPointF(getC0Frame(), getC0Value());
        fromPt = QPointF(getC1Frame(), getC1Value());
        targetPt = mC0Point.get();
    } else {
        fromPt = QPointF(getC0Frame(), getC0Value());
        toPt = QPointF(getC1Frame(), getC1Value());
        targetPt = mC1Point.get();
    }
    QPointF newFrameValue;
    const QPointF graphPt(mRelFrame, getValueForGraph());
    if(mCtrlsMode == CtrlsMode::smooth) {
        // mFrame and mValue are of different units chence len is wrong
        newFrameValue = symmetricToPosNewLen(fromPt, graphPt,
                                             pointToLen(toPt - graphPt));

    } else if(mCtrlsMode == CtrlsMode::symmetric) {
        newFrameValue = symmetricToPos(fromPt, graphPt);
    }
    targetPt->setValue(newFrameValue.y());
    targetPt->setRelFrame(newFrameValue.x());

    mParentAnimator->anim_updateAfterChangedKey(this);
}

void GraphKey::setCtrlsMode(const CtrlsMode mode) {
    mCtrlsMode = mode;
    const QPointF pos(mRelFrame, getValueForGraph());
    QPointF c0Pos(getC0Frame(), getC0Value());
    QPointF c1Pos(getC1Frame(), getC1Value());
    if(mCtrlsMode == CtrlsMode::symmetric) {
        gGetCtrlsSymmetricPos(c0Pos, pos, c1Pos, c0Pos, c1Pos);
    } else if(mCtrlsMode == CtrlsMode::smooth) {
        gGetCtrlsSmoothPos(c0Pos, pos, c1Pos, c0Pos, c1Pos);
    } else return;
    setC0Frame(c0Pos.x());
    setC0Value(c0Pos.y());
    setC1Frame(c1Pos.x());
    setC1Value(c1Pos.y());
}

CtrlsMode GraphKey::getCtrlsMode() const {
    return mCtrlsMode;
}

void GraphKey::drawGraphKey(QPainter *p, const QColor &paintColor) const {
    if(isSelected()) {
        p->save();
        QPen pen(Qt::black, 1.5);
        pen.setCosmetic(true);

        QPen pen2(Qt::white, .75);
        pen2.setCosmetic(true);

        const QPointF thisPos(getAbsFrame(), getValueForGraph());
        if(getC0Enabled()) {
            const QPointF c0Pos(getC0AbsFrame(), getC0Value());
            p->setPen(pen);
            p->drawLine(thisPos, c0Pos);
            p->setPen(pen2);
            p->drawLine(thisPos, c0Pos);
        }
        if(getC1Enabled()) {
            const QPointF c1Pos(getC1AbsFrame(), getC1Value());
            p->setPen(pen);
            p->drawLine(thisPos, c1Pos);
            p->setPen(pen2);
            p->drawLine(thisPos, c1Pos);
        }
        p->restore();
    }
    mKeyPoint->draw(p, paintColor);
    if(isSelected()) {
        if(getC0Enabled() && hasPrevKey()) {
            mC0Point->draw(p, paintColor);
        }
        if(getC1Enabled() && hasNextKey()) {
            mC1Point->draw(p, paintColor);
        }
    }
}

void GraphKey::startFrameAndValueTransform() {
    startFrameTransform();
    startValueTransform();
}

void GraphKey::finishFrameAndValueTransform() {
    finishFrameTransform();
    finishValueTransform();
}

void GraphKey::cancelFrameAndValueTransform() {
    cancelFrameTransform();
    cancelValueTransform();
}

void GraphKey::constrainC0Value(const qreal minVal, const qreal maxVal) {
    mC0Clamped.setYRange(minVal, maxVal);
//    if(!getc0EnabledForGraph()) return;
//    const qreal c0Value = getc0Value();
//    if(c0Value > minVal && c0Value < maxVal) return;
//    const qreal newValue = clamp(c0Value, minVal, maxVal);
//    mc0Point->moveTo(getc0Frame(), newValue);
}

void GraphKey::constrainC1Value(const qreal minVal, const qreal maxVal) {
    mC1Clamped.setYRange(minVal, maxVal);
    //    if(!getc1EnabledForGraph()) return;
//    const qreal c1Value = getc1Value();
//    if(c1Value > minVal && c1Value < maxVal) return;
//    const qreal newValue = clamp(c1Value, minVal, maxVal);
//    mc1Point->moveTo(getc1Frame(), newValue);
}

void GraphKey::constrainC0MinFrame(const qreal minRelFrame) {
    mC0Clamped.setXRange(minRelFrame, mRelFrame);
//    const qreal c0Frame = getc0Frame();
//    if(c0Frame > minFrame || !getc0EnabledForGraph()) return;
//    const qreal c0Value = getc0Value();
//    const qreal value = getValueForGraph();
//    const qreal newFrame = clamp(c0Frame, qreal(minFrame), qreal(mRelFrame));
//    const qreal change = (mRelFrame - newFrame)/(mRelFrame - c0Frame);
//    mc0Point->moveTo(newFrame, change*(c0Value - value) + value);
}

void GraphKey::constrainC1MaxFrame(const qreal maxRelFrame) {
    mC1Clamped.setXRange(mRelFrame, maxRelFrame);
//    const qreal c1Frame = getc1Frame();
//    if(c1Frame < maxFrame || !getc1EnabledForGraph()) return;
//    const qreal c1Value = getc1Value();
//    const qreal value = getValueForGraph();
//    const qreal newFrame = clamp(c1Frame, qreal(mRelFrame), qreal(maxFrame));
//    const qreal change = (newFrame - mRelFrame)/(c1Frame - mRelFrame);
//    mc1Frame.setRange(mRelFrame, maxFrame);
    //mc1Point->moveTo(newFrame, change*(c1Value - value) + value);
}

qreal GraphKey::getPrevKeyValueForGraph() const {
    auto prevKey = getPrevKey<GraphKey>();
    if(!prevKey) return getValueForGraph();
    return prevKey->getValueForGraph();
}

qreal GraphKey::getNextKeyValueForGraph() const {
    auto nextKey = getNextKey<GraphKey>();
    if(!nextKey) return getValueForGraph();
    return nextKey->getValueForGraph();
}

bool GraphKey::isInsideRect(const QRectF &valueFrameRect) const {
    const QPointF keyPoint(getAbsFrame(), getValueForGraph());
    return valueFrameRect.contains(keyPoint);
}

void GraphKey::makeC0C1Smooth() {
    const qreal nextKeyVal = getNextKeyValueForGraph();
    const qreal prevKeyVal = getPrevKeyValueForGraph();
    const int nextKeyFrame = getNextKeyRelFrame();
    const int prevKeyFrame = getPrevKeyRelFrame();
    qreal valIncPerFrame;
    if(nextKeyFrame == mRelFrame || prevKeyFrame == mRelFrame) {
        valIncPerFrame = 0;
    } else {
        valIncPerFrame =
                (nextKeyVal - prevKeyVal)/(nextKeyFrame - prevKeyFrame);
    }
    const qreal newc0Val = getValueForGraph() +
            (getC0Frame() - mRelFrame)*valIncPerFrame;
    const qreal newc1Val = getValueForGraph() +
            (getC1Frame() - mRelFrame)*valIncPerFrame;
    setC0Value(newc0Val);
    setC1Value(newc1Val);
}


void GraphKey::setC0Enabled(const bool bT) {
    mC0Enabled = bT;
}

void GraphKey::setC1Enabled(const bool bT) {
    mC1Enabled = bT;
}

qreal GraphKey::getC0Frame() const {
    if(mC0Enabled) {
        const QPointF relTo{qreal(mRelFrame), getValueForGraph()};
        return mC0Clamped.getClampedValue(relTo).x();
    }
    return mRelFrame;
}

qreal GraphKey::getC1Frame() const {
    if(mC1Enabled) {
        const QPointF relTo{qreal(mRelFrame), getValueForGraph()};
        return mC1Clamped.getClampedValue(relTo).x();
    }
    return mRelFrame;
}

qreal GraphKey::getC0AbsFrame() const {
    return relFrameToAbsFrameF(getC0Frame());
}

qreal GraphKey::getC1AbsFrame() const {
    return relFrameToAbsFrameF(getC1Frame());
}

bool GraphKey::getC0Enabled() const {
    return mC0Enabled;
}

bool GraphKey::getC1Enabled() const {
    return mC1Enabled;
}

void GraphKey::setC0FrameVar(const qreal c0Frame) {
    mC0Clamped.setXValue(c0Frame);
}

void GraphKey::setC1FrameVar(const qreal c1Frame) {
    mC1Clamped.setXValue(c1Frame);
}

void GraphKey::setC0Frame(const qreal c0Frame) {
    setC0FrameVar(c0Frame);
    mParentAnimator->anim_updateAfterChangedKey(this);
}

void GraphKey::setC1Frame(const qreal c1Frame) {
    setC1FrameVar(c1Frame);
    mParentAnimator->anim_updateAfterChangedKey(this);
}

void GraphKey::setRelFrame(const int frame) {
    if(frame == mRelFrame) return;
    const int dFrame = frame - mRelFrame;
    setC0FrameVar(mC0Clamped.getRawXValue() + dFrame);
    setC1FrameVar(mC1Clamped.getRawXValue() + dFrame);
    mC0Clamped.setXMax(frame);
    mC1Clamped.setXMin(frame);
    mRelFrame = frame;
}

void GraphKey::setC0ValueVar(const qreal value) {
    mC0Clamped.setYValue(value);
}

void GraphKey::setC1ValueVar(const qreal value) {
    mC1Clamped.setYValue(value);
}

void GraphKey::setC0Value(const qreal value) {
    setC0ValueVar(value);
    mParentAnimator->anim_updateAfterChangedKey(this);
}

void GraphKey::setC1Value(const qreal value) {
    setC1ValueVar(value);
    mParentAnimator->anim_updateAfterChangedKey(this);
}

qreal GraphKey::getC0Value() const {
    if(mC0Enabled) {
        const QPointF relTo{qreal(mRelFrame), getValueForGraph()};
        return mC0Clamped.getClampedValue(relTo).y();
    }
    return getValueForGraph();
}

qreal GraphKey::getC1Value() const {
    if(mC1Enabled) {
        const QPointF relTo{qreal(mRelFrame), getValueForGraph()};
        return mC1Clamped.getClampedValue(relTo).y();
    }
    return getValueForGraph();
}

void GraphKey::saveC0C1Value() {
    mC0Clamped.saveYValue();
    mC1Clamped.saveYValue();
}
