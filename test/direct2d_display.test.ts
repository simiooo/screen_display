import { Direct2DDisplay } from '../src/main';
import { expect } from 'chai';

describe('Direct2DDisplay', () => {
  let display: Direct2DDisplay;

  beforeEach(() => {
    display = new Direct2DDisplay();
  });

  afterEach(() => {
    display.stop();
  });

  describe('#start()', () => {
    it('should initialize display with default text', () => {
      const result = display.start("Test Text");
      expect(result).to.be.true;
    });

    it('should initialize display on specified monitor', () => {
      const result = display.start("Test Text", 0);
      expect(result).to.be.true;
    });

    it('should return false for invalid monitor index', () => {
      const result = display.start("Test Text", -1);
      expect(result).to.be.false;
    });
  });

  describe('#updateText()', () => {
    it('should update displayed text', () => {
      display.start("Initial Text");
      display.updateText("Updated Text");
      // Add assertions for text update verification
    });

    it('should handle empty string', () => {
      display.start("Initial Text");
      display.updateText("");
      // Add assertions for empty text verification
    });
  });

  describe('#updatePosition()', () => {
    it('should update text position', () => {
      display.start("Test Text");
      display.updatePosition(100, 200);
      // Add assertions for position verification
    });

    it('should handle negative coordinates', () => {
      display.start("Test Text");
      display.updatePosition(-100, -200);
      // Add assertions for negative position verification
    });
  });

  describe('#updateStyle()', () => {
    it('should update text style', () => {
      display.start("Test Text");
      display.updateStyle(32, 400);
      // Add assertions for style verification
    });

    it('should handle invalid font weights', () => {
      display.start("Test Text");
      display.updateStyle(32, 950); // Invalid weight
      // Add assertions for invalid weight handling
    });
  });

  describe('#stop()', () => {
    it('should stop display', () => {
      display.start("Test Text");
      const result = display.stop();
      expect(result).to.be.true;
    });

    it('should handle multiple stop calls', () => {
      display.start("Test Text");
      display.stop();
      const result = display.stop();
      expect(result).to.be.false;
    });
  });
});
