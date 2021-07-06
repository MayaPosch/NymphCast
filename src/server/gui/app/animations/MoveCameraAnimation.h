#pragma once
#ifndef ES_APP_ANIMATIONS_MOVE_CAMERA_ANIMATION_H
#define ES_APP_ANIMATIONS_MOVE_CAMERA_ANIMATION_H

#include "animations/Animation.h"

class MoveCameraAnimation : public Animation
{
public:
	MoveCameraAnimation(Transform4x4f& camera, const Vector3f& target) : mCameraStart(camera), mTarget(target), cameraOut(camera) {}

	int getDuration() const override { return 400; }

	void apply(float t) override
	{
		// cubic ease out
		t -= 1;
		cameraOut.translation() = -Vector3f().lerp(-mCameraStart.translation(), mTarget, t*t*t + 1);
	}

private:
	Transform4x4f mCameraStart;
	Vector3f mTarget;

	Transform4x4f& cameraOut;
};

#endif // ES_APP_ANIMATIONS_MOVE_CAMERA_ANIMATION_H
