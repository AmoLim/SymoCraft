//
// Created by Amo on 2022/7/4.
//

#ifndef SYMOCRAFT_EVENT_H
#define SYMOCRAFT_EVENT_H

#include "core.h"

namespace SymoCraft::EventSystem
{

    const std::string recordpath_test = "../assets/record";

    namespace ECS
    {
        class Registry;
    }

    enum class EventType : uint8
    {
        None = 0,
        PlayerKeyInput,
        PlayMouseInput,
        PlayerMouseButtonInput,
        PlayerMouseScrollInput
    };

    struct Event
    {
        EventType type;
        size_t size;
        void *data;
        bool is_free; // if the event is used, free the memory
    };

    class EventListener
    {
    public:
        void Init(ECS::Registry &application_registry);

        // EventListener Update
        void Update();

        // Queue Main Event into events
        void QueueMainEvent(EventType event_type, void *event_data = nullptr
                            , size_t event_data_size = 0, bool free_data = false);
        void QueueMainEvent_Key(int key, int action);
        void QueueMainEvent_Mouse(float xpos, float ypos);
        void QueueMainEvent_MouseButton(int button, int action);

        // Process the series of events
        void ProcessEvents();

        // Process a single event
        // Parameters: EventType, data pointer, size of data
        void ProcessEvent(EventType type, void *data, size_t size_of_data);

        // Serialize an event to the target file
        // Parameters: a Event reference
        void SerializeEvent(const Event &event);

    private:
        bool serialize_events;
        bool play_from_eventfile;
        std::queue<Event> events;
        FILE* serialized_eventfile;
        ECS::Registry *registry;
    };
}

#endif //SYMOCRAFT_EVENT_H
