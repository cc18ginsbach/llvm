#ifndef _GRAPHENGINE_HPP_
#define _GRAPHENGINE_HPP_
#include <climits>
#include <vector>

/* This class implements helper functions to calculate fast graph domination tests. */
class GraphEngine
{
public:
    GraphEngine(size_t size) : counter(UINT_MAX), working_status(size, 0), working_queue(size, 0) { }

    GraphEngine() = default;

    unsigned              counter;
    std::vector<unsigned> working_status;
    std::vector<unsigned> working_queue;
    unsigned*             queue_front;
    unsigned*             queue_back;

    inline void initialize()
    {
        ++counter;
        queue_front = &working_queue[0];
        queue_back  = &working_queue[0];
    }

    template<typename iter_type>
    inline void set_destinations(iter_type destinations_begin, iter_type destinations_end)
    {
        for(auto destination = destinations_begin; destination != destinations_end; ++destination)
        {
            working_status[*destination] = 5*counter+2;
        }
    }

    template<typename iter_type>
    inline void set_dominators(iter_type dominators_begin, iter_type dominators_end)
    {
        for(auto dominator = dominators_begin; dominator != dominators_end; ++dominator)
        {
            working_status[*dominator] = 5*counter+1;
        }
    }

    template<typename iter_type>
    inline bool set_origins(iter_type origins_begin, iter_type origins_end)
    {
        for(auto origin = origins_begin; origin != origins_end; ++origin)
        {
            if(working_status[*origin] == 5*counter+2)
            {
                return false;
            }
            if(working_status[*origin] <= 5*counter)
            {
                working_status[*origin] = 5*counter+3;
                *(queue_back++) = *origin;
            }
        }

        return true;
    }

    inline bool fill(const std::vector<std::vector<unsigned>>& graph)
    {
        while(queue_front != queue_back)
        {
            auto element = *(queue_front++);
    
            for(unsigned i : graph[element])
            {
                if(working_status[i] == 5*counter+2)
                {
                    return false;
                }
                if(working_status[i] <= 5*counter)
                {
                    working_status[i] = 5*counter+3;
                    *(queue_back++) = i;
                }
            }
        }

        return true;
    }
};

#endif
