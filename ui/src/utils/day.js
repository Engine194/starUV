export const daysMap = new Map([
  ["en", ["MO", "TU", "WE", "TH", "FR", "SA", "SU"]],
  ["vi", ["T2", "T3", "T4", "T5", "T6", "T7", "CN"]],
]);

export const timeSelections = {
  hour: Array.from({ length: 24 }).map((_, index) => index),
  minute: Array.from({ length: 60 }).map((_, index) => index),
  second: Array.from({ length: 60 }).map((_, index) => index),
};

export const combineTimeComponents = (components) => {
  const { hour = 0, minute = 0, second = 0 } = components || {};
  return `${hour.toString().padStart(2, "0")}:${minute
    .toString()
    .padStart(2, "0")}:${second.toString().padStart(2, "0")}`;
};

/**
 * Parses a time string in the format 'hh:mm:ss' and returns an object
 * with numeric hour, minute, and second properties.
 *
 * @param {string} timeString - The time string to parse (e.g., '12:34:56').
 * @returns {{ hour: number, minute: number, second: number }} The parsed time components.
 */
export const getTimeComponents = (timeString) => {
  // timeString: hh:mm:ss
  const components = timeString.split(":");
  return {
    hour: Number(components[0]),
    minute: Number(components[1]),
    second: Number(components[2]),
  };
};
