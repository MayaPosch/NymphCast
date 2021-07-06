#pragma once
#ifndef ES_APP_ANIMATIONS_LAUNCH_ANIMATION_H
#define ES_APP_ANIMATIONS_LAUNCH_ANIMATION_H

#include "animations/Animation.h"

// let's look at the game launch effect:
// -move camera to center on point P (interpolation method: linear)
// -zoom camera to factor Z via matrix scale (interpolation method: exponential)
// -fade screen to black at rate F (interpolation method: exponential)

// How the animation gets constructed from the example of implementing the game launch effect:
// 1. Current parameters are retrieved through a get() call
//    ugliness:
//		-have to have a way to get a reference to the animation
//      -requires additional implementation in some parent object, potentially AnimationController
// 2. ViewController is passed in LaunchAnimation constructor, applies state through setters
//    ugliness:
//      -effect only works for ViewController
// 3. Pass references to ViewController variables - LaunchAnimation(mCameraPos, mFadePerc, target)
//    ugliness:
//      -what if ViewController is deleted? --> AnimationController class handles that
//      -no callbacks for changes...but that works well with this style of update, so I think it's okay
// 4. Use callbacks to set variables
//    ugliness:
//      -boilerplate as hell every time

// #3 wins
// can't wait to see how this one bites me in the ass

class LaunchAnimation : public Animation
{
public:
	//Target is a centerpoint
	LaunchAnimation(Transform4x4f& camera, float& fade, const Vector3f& target, int duration) :
	  mCameraStart(camera), mTarget(target), mDuration(duration), cameraOut(camera), fadeOut(fade) {}

	int getDuration() const override { return mDuration; }

	void apply(float t) override
	{
		cameraOut = Transform4x4f::Identity();

		float zoom = Math::lerp(1.0, 4.25f, t*t);
		cameraOut.scale(zoom);

		const float sw = (float)Renderer::getScreenWidth() / zoom;
		const float sh = (float)Renderer::getScreenHeight() / zoom;

		const Vector2f startPoint  = Vector2f(-mCameraStart.translation()) + Vector2f(Renderer::getScreenWidth() / 2.0f, Renderer::getScreenHeight() / 2.0f);
		const Vector2f centerPoint = Vector2f().lerp(startPoint, Vector2f(mTarget), Math::smootherStep(0.0, 1.0, t));

		cameraOut.translate(Vector3f((sw / 2) - centerPoint.x(), (sh / 2) - centerPoint.y(), 0));

		fadeOut = Math::lerp(0.0, 1.0, t*t);
	}

private:
	Transform4x4f mCameraStart;
	Vector3f mTarget;
	int mDuration;

	Transform4x4f& cameraOut;
	float& fadeOut;
};

#endif // ES_APP_ANIMATIONS_LAUNCH_ANIMATION_H
