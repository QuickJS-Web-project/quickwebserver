export const getType = (variable) => {
  const typeString = Object.prototype.toString.call(variable);
  return typeString.replace(/(\W)|(object)/g, '').toLowerCase();
};
