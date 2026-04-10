declare module "juce-framework-frontend" {
  interface ListenerList {
    addListener(fn: (...args: any[]) => void): number;
    removeListener(id: number): void;
  }

  interface SliderState {
    getNormalisedValue(): number;
    setNormalisedValue(v: number): void;
    sliderDragStarted(): void;
    sliderDragEnded(): void;
    valueChangedEvent: ListenerList;
    propertiesChangedEvent: ListenerList;
    properties: {
      start: number;
      end: number;
      skew: number;
      name: string;
      label: string;
      numSteps: number;
      interval: number;
      parameterIndex: number;
    };
  }

  interface ToggleState {
    getValue(): boolean;
    setValue(v: boolean): void;
    valueChangedEvent: ListenerList;
  }

  export function getSliderState(name: string): SliderState;
  export function getToggleState(name: string): ToggleState;
  export function getComboBoxState(name: string): any;
  export function getNativeFunction(name: string): (...args: any[]) => Promise<any>;
  export function getBackendResourceAddress(resource: string): string;
}
