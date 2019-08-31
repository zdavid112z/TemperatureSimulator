#pragma once

#include "Pch.h"

struct EventLocation {
	EventLocation() {}

	EventLocation(uint x, uint y) : x(x), y(y) {}

	uint x, y;

	bool operator==(const EventLocation& other) const
	{
		return x == other.x && y == other.y;
	}
};

namespace std {

	template <>
	struct hash<EventLocation>
	{
		std::size_t operator()(const EventLocation& k) const
		{
			return (std::hash<uint>()(k.x)) ^ (std::hash<uint>()(k.y) * 31);
		}
	};

}

struct Event {
	Event() {}
	Event(uint triggerTime, EventLocation location, bool start, std::function<float(uint)> calcFunction) :
		start(start),
		triggerTime(triggerTime),
		location(location),
		calcFunction(calcFunction) {}

	bool start;
	std::function<float(uint)> calcFunction;
	uint triggerTime;
	EventLocation location;

	bool operator<(const Event& event) const {
		return (triggerTime == event.triggerTime && start && !event.start) ||
			triggerTime > event.triggerTime;
	}
};