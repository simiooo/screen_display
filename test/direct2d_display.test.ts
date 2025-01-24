import { createRequire } from 'module';
const require = createRequire(import.meta.url);
const { Direct2DDisplay } = require('../dist/main');
import { expect } from 'chai';

describe('Direct2DDisplay', () => {
  let display: typeof Direct2DDisplay;

  beforeEach(() => {
    display = new Direct2DDisplay();
  });

  afterEach(() => {
    display.stop();
  });

  describe('#start()', () => {
    it('should initialize display', () => {
      const result = display.start(0);
      expect(result).to.be.true;
    });

    it('should throw for invalid monitor index', () => {
      expect(() => display.start(-1)).to.throw('Invalid monitor index');
    });

    it('should throw when already started', () => {
      display.start(0);
      expect(() => display.start(0)).to.throw('Display already started');
    });
  });

  describe('#updateAll()', () => {
    it('should update all parameters', () => {
      display.start(0);
      display.updateAll(100, 200, "Test Text");
      // Add assertions for parameter verification
    });

    it('should throw when not started', () => {
      expect(() => display.updateAll(100, 200, "Test Text"))
        .to.throw('Display not started');
    });

    it('should throw for invalid text', () => {
      display.start(0);
      expect(() => display.updateAll(100, 200, ""))
        .to.throw('Text cannot be empty');
    });
  });

  describe('#stop()', () => {
    it('should stop display', () => {
      display.start(0);
      const result = display.stop();
      expect(result).to.be.true;
    });

    it('should throw when not started', () => {
      expect(() => display.stop()).to.throw('Display not started');
    });

    it('should handle multiple stop calls', () => {
      display.start(0);
      display.stop();
      expect(() => display.stop()).to.throw('Display already stopped');
    });
  });
});
