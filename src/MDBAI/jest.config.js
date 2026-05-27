/**
 * MDBAI — Configuration Jest (ESM Node.js 20)
 * Utiliser: NODE_OPTIONS=--experimental-vm-modules npx jest
 */
export default {
  testEnvironment: 'node',
  transform: {},
  testMatch: [
    '**/tests/**/*.test.js',
  ],
  testPathIgnorePatterns: ['/node_modules/'],
  setupFilesAfterEnv: ['<rootDir>/tests/setup.js'],
  verbose: true,
  testTimeout: 30000,
  forceExit: true,
  collectCoverageFrom: [
    'src/**/*.js',
    '!src/server.js',
    '!**/node_modules/**',
  ],
};
