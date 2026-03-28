import { Canvas } from "@react-three/fiber";
import { ACESFilmicToneMapping } from "three";
import type { ReactNode } from "react";

interface ThreeCanvasProps {
  children: ReactNode;
  /**
   * CSS width of the canvas container. Defaults to "100%".
   */
  width?: string;
  /**
   * CSS height of the canvas container. Defaults to "100%".
   */
  height?: string;
  /**
   * Camera Z distance. Defaults to 5.
   */
  cameraZ?: number;
}

/**
 * Drop-in Canvas wrapper for React Three Fiber scenes.
 *
 * Usage — wrap your 3D scene components in <ThreeCanvas>:
 *
 *   import ThreeCanvas from "./r3f/ThreeCanvas";
 *
 *   <ThreeCanvas>
 *     <ambientLight intensity={0.5} />
 *     <mesh>
 *       <boxGeometry />
 *       <meshStandardMaterial color="hotpink" />
 *     </mesh>
 *   </ThreeCanvas>
 *
 * The canvas fills its parent container, so control size via the parent div.
 * See ExampleScene.tsx for a complete working scene.
 */
export default function ThreeCanvas({
  children,
  width = "100%",
  height = "100%",
  cameraZ = 5,
}: ThreeCanvasProps) {
  return (
    <div style={{ width, height, position: "relative" }}>
      <Canvas
        camera={{ position: [0, 0, cameraZ], fov: 60 }}
        gl={{ toneMapping: ACESFilmicToneMapping, toneMappingExposure: 1 }}
        style={{ width: "100%", height: "100%" }}
      >
        {children}
      </Canvas>
    </div>
  );
}
