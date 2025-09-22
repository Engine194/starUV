import { STORE_ACTION_TYPES } from "../actions/storeContext.action";

export const initState = {
  language: "vi",
  loading: false,
  cycles: [
    {
      id: 1,
      start: "06:15:00",
      end: "06:45:00",
      day: [1, 0, 1, 0, 1, 0, 1],
      fan_delay: 30,
      fan_enable: 1,
    },
    {
      id: 2,
      start: "08:00:00",
      end: "08:30:00",
      day: [0, 1, 0, 1, 0, 1, 0],
      fan_delay: 35,
      fan_enable: 1,
    },
    {
      id: 3,
      start: "10:30:00",
      end: "11:00:00",
      day: [1, 1, 0, 0, 1, 1, 0],
      fan_delay: 35,
      fan_enable: 1,
    },
    {
      id: 4,
      start: "13:00:00",
      end: "13:30:00",
      day: [0, 0, 1, 1, 0, 0, 1],
      fan_delay: 30,
      fan_enable: 1,
    },
    {
      id: 5,
      start: "15:45:00",
      end: "16:15:00",
      day: [1, 1, 1, 1, 1, 1, 1],
      fan_delay: 30,
      fan_enable: 1,
    },
  ],
};

export const storeReducer = (state, action) => {
  const { type, payload } = action;
  switch (type) {
    case STORE_ACTION_TYPES.GET_LANGUAGE:
    case STORE_ACTION_TYPES.UPDATE_LANGUAGE:
      return { ...state, language: payload };
    case STORE_ACTION_TYPES.START_LOADING:
      return { ...state, loading: true };
    case STORE_ACTION_TYPES.END_LOADING:
      return { ...state, loading: false };
    case STORE_ACTION_TYPES.GET_CYCLES:
      return { ...state, cycles: [...(payload || [])] };
    case STORE_ACTION_TYPES.ADD_CYCLE:
      return { ...state, cycles: [...state.cycles, payload] };
    case STORE_ACTION_TYPES.UPDATE_CYCLE: {
      const cycleId = payload.id;
      const findCycle = state.cycles.findIndex(({ id }) => id === cycleId);
      if (findCycle >= 0) {
        const nextCycles = [...state.cycles];
        nextCycles.splice(findCycle, 1, { ...payload });
        return { ...state, cycles: nextCycles };
      } else {
        return state;
      }
    }
    default:
      return state;
  }
};
