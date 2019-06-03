#ifndef ANIMATEDSURFACE_H
#define ANIMATEDSURFACE_H
#include "Animators/graphanimator.h"
#include "Animators/interpolationkeyt.h"
#include "drawableautotiledsurface.h"
#include "Animators/qrealpoint.h"
class AnimatedSurface;
class ASKey : public GraphKey {
    friend class StdSelfRef;
protected:
    ASKey(AnimatedSurface * const parent);
    ASKey(const int& frame, AnimatedSurface * const parent);
    ASKey(const DrawableAutoTiledSurface &value,
          const int& frame, AnimatedSurface * const parent);
public:
    bool differsFromKey(Key * const key) const {
        return key != this;
    }

    qreal getValueForGraph() const {
        return mRelFrame;
    }

    void setValueForGraph(const qreal& value) {
        Q_UNUSED(value);
    }

    void setRelFrame(const int &frame) {
        if(frame == mRelFrame) return;
        const int dFrame = frame - mRelFrame;
        GraphKey::setRelFrame(frame);
        setStartFrameVar(mStartPt.getRawXValue() + dFrame);
        setEndFrameVar(mEndPt.getRawXValue() + dFrame);
    }

    DrawableAutoTiledSurface& dSurface() { return *mValue.get(); }
private:
    const stdsptr<DrawableAutoTiledSurface> mValue;
};

class AnimatedSurface : public GraphAnimator {
    Q_OBJECT
    friend class SelfRef;
    //typedef InterpolationKeyT<AutoTiledSurface> ASKey;
protected:
    AnimatedSurface();
public:
    struct OnionSkin {
        struct Skin {
            SkScalar fX;
            SkScalar fY;
            SkBitmap fBtmp;
            SkScalar fWeight;
        };

        struct SkinsSide {
            SkColor4f fColor;
            QList<Skin> fSkins;

            void draw(SkCanvas * const canvas) {
                for(const auto& skin : fSkins) {
                    SkPaint paint;
                    const SkScalar wAlpha = -0.25f;
                    const SkScalar opacityM[20] = {
                        1, 0, 0, 0, 0,
                        0, 1, 0, 0, 0,
                        0, 0, 1, 0, 0,
                        wAlpha, wAlpha, wAlpha, skin.fWeight + wAlpha, 0};
                    const auto opacityF = SkColorFilters::Matrix(opacityM);
                    const SkScalar colM[20] = {
                        0, 0, 0, fColor.fR, 0,
                        0, 0, 0, fColor.fG, 0,
                        0, 0, 0, fColor.fB, 0,
                        0, 0, 0, fColor.fA, 0};
                    const auto colF = SkColorFilters::Matrix(colM);
                    const auto totF = SkColorFilters::Compose(colF, opacityF);
                    paint.setColorFilter(totF);

                    canvas->drawBitmap(skin.fBtmp, skin.fX, skin.fY, &paint);
                }
            }

            void clear() { fSkins.clear(); }
        };

        SkinsSide fPrev{{0, 0, 1, 1}, QList<Skin>()};
        SkinsSide fNext{{1, 0, 0, 1}, QList<Skin>()};

        void draw(SkCanvas * const canvas) {
            fPrev.draw(canvas);
            fNext.draw(canvas);
        }

        void clear() { fPrev.clear(); fNext.clear(); }
    };

    void setupOnionSkinFor(const int sideRange, OnionSkin &skins) {
        setupOnionSkinFor(anim_getCurrentRelFrame(), sideRange, skins);
    }

    void setupOnionSkinFor(const int relFrame, const int sideRange,
                           OnionSkin &skins) {
        skins.clear();
        ASKey * currKey = anim_getKeyAtRelFrame<ASKey>(relFrame);
        if(!currKey) currKey = anim_getPrevKey<ASKey>(relFrame);
        if(!currKey) currKey = anim_getNextKey<ASKey>(relFrame);
        if(!currKey) return;
        ASKey * prevKey = anim_getPrevKey<ASKey>(currKey);
        while(prevKey) {
            const auto& surf = prevKey->dSurface().surface();
            const auto bitmap = surf.toBitmap();
            const int dFrame = qAbs(relFrame - prevKey->getRelFrame());
            if(dFrame > sideRange) break;
            const qreal weight = 1.*(sideRange - dFrame)/sideRange;
            const QRect pixRect = surf.pixelBoundingRect();
            skins.fPrev.fSkins.append({toSkScalar(pixRect.x()),
                                       toSkScalar(pixRect.y()),
                                       bitmap, toSkScalar(weight)});
            prevKey = anim_getPrevKey<ASKey>(prevKey);
        }

        ASKey * nextKey = anim_getNextKey<ASKey>(currKey);
        while(nextKey) {
            const auto& surf = nextKey->dSurface().surface();
            const auto bitmap = surf.toBitmap();
            const int dFrame = qAbs(relFrame - nextKey->getRelFrame());
            if(dFrame > sideRange) break;
            const qreal weight = 1.*(sideRange - dFrame)/sideRange;
            const QRect pixRect = surf.pixelBoundingRect();
            skins.fNext.fSkins.append({toSkScalar(pixRect.x()),
                                       toSkScalar(pixRect.y()),
                                       bitmap, toSkScalar(weight)});
            nextKey = anim_getNextKey<ASKey>(nextKey);
        }
    }

    stdsptr<Key> readKey(QIODevice *target) {
        auto newKey = SPtrCreate(ASKey)(this);
        newKey->readKey(target);
        return std::move(newKey);
    }

    void anim_saveCurrentValueAsKey() {
        anim_addKeyAtRelFrame(anim_getCurrentRelFrame());
    }

    void anim_addKeyAtRelFrame(const int& relFrame) {
        if(anim_getKeyAtRelFrame(relFrame)) return;
        const auto prevNextKey = anim_getPrevAndNextKey<ASKey>(relFrame);
        stdsptr<ASKey> newKey;
        if(prevNextKey.first) {
            const auto& value = prevNextKey.first->dSurface();
            newKey = SPtrCreate(ASKey)(value, relFrame, this);
        } else if(prevNextKey.second) {
            const auto& value = prevNextKey.second->dSurface();
            newKey = SPtrCreate(ASKey)(value, relFrame, this);
        } else {
            newKey = SPtrCreate(ASKey)(*mBaseValue.get(), relFrame, this);
        }
        anim_appendKey(newKey);
    }

    void anim_afterKeyOnCurrentFrameChanged(Key* const key) {
        const auto spk = static_cast<ASKey*>(key);
        if(spk) setCurrent(&spk->dSurface());
        else {
            const auto relFrame = anim_getCurrentRelFrame();
            const auto prevNextKey = anim_getPrevAndNextKey<ASKey>(relFrame);
            if(prevNextKey.first) {
                setCurrent(&prevNextKey.first->dSurface());
            } else if(prevNextKey.second) {
                setCurrent(&prevNextKey.second->dSurface());
            } else {
                setCurrent(mBaseValue.get());
            }
        }
    }

    void graph_getValueConstraints(
            GraphKey *key, const QrealPointType &type,
            qreal &minValue, qreal &maxValue) const {
        if(type == QrealPointType::KEY_POINT) {
            minValue = key->getRelFrame();
            maxValue = minValue;
            //getFrameConstraints(key, type, minValue, maxValue);
        } else {
            minValue = -DBL_MAX;
            maxValue = DBL_MAX;
        }
    }

    DrawableAutoTiledSurface * getCurrentSurface() {
        return mCurrent_d;
    }


    void newEmptyFrame() {
        newEmptyFrame(anim_getCurrentRelFrame());
    }

    void newEmptyFrame(const int relFrame) {
        if(anim_getKeyAtRelFrame(relFrame)) return;
        const auto newKey = SPtrCreate(ASKey)(relFrame, this);
        anim_appendKey(newKey);
    }
signals:
    void currentSurfaceChanged(DrawableAutoTiledSurface*);
private:
    void setCurrent(DrawableAutoTiledSurface * const surf) {
        if(mCurrent_d == surf) return;
        mCurrent_d = surf;
        emit currentSurfaceChanged(mCurrent_d);
    }

    const stdsptr<DrawableAutoTiledSurface> mBaseValue;
    DrawableAutoTiledSurface * mCurrent_d = nullptr;
};

#endif // ANIMATEDSURFACE_H
