//
// Created by Amo on 2022/7/4.
//
#include "event/event.h"
#include <cstdio>
#include "core/ECS/component.h"
#include "core/application.h"
#include "camera/camera.h"

namespace SymoCraft
{
    namespace EventSystem
    {
        void EventListener::Init(ECS::Registry &application_registry)
        {
            serialized_eventfile = nullptr;

            registry = &application_registry;
        }

        void EventListener::Update()
        {
            // Process events and open the event file if needed
            if (serialized_eventfile == nullptr && (serialize_events || play_from_eventfile))
            {
                std::string event_file_path = recordpath_test + "/replay.bin";

                if (serialize_events)
                    serialized_eventfile = fopen(event_file_path.c_str(), "wb");
                else
                    serialized_eventfile = fopen(event_file_path.c_str(), "rb");

                if (!serialized_eventfile)
                {
                    serialize_events = false;
                    play_from_eventfile = false;
                    AmoLogger_Error("Could not open file path '%s' to serialize world events.", event_file_path.c_str());
                    serialized_eventfile = nullptr;
                }
            }
        }

        void EventListener::QueueMainEvent(EventType event_type, void *event_data, size_t event_data_size,
                                           bool free_data)
        {
            if (!play_from_eventfile)
            {
                Event new_event;
                new_event.type = event_type;
                new_event.data = event_data;
                new_event.size = event_data_size;

            }
        }
    }
}


