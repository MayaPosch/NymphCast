#pragma once
#ifndef ES_CORE_ANIMATIONS_ANIMATION_H
#define ES_CORE_ANIMATIONS_ANIMATION_H

class Animation
{
public:
	virtual int getDuration() const = 0;
	virtual void apply(float t) = 0;
};

#endif // ES_CORE_ANIMATIONS_ANIMATION_H
