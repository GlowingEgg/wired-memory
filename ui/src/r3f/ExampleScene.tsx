import { useRef } from "react";
import { useFrame } from "@react-three/fiber";
import type { Mesh } from "three";
import ThreeCanvas from "./ThreeCanvas";

/**
 * ExampleScene — a minimal React Three Fiber scene showing the standard
 * pattern for this template.
 *
 * This file is NOT imported anywhere by default. It exists as a reference.
 * To activate it, add it to App.tsx:
 *
 *   import ExampleScene from "./r3f/ExampleScene";
 *   // then inside your JSX:
 *   <ExampleScene />
 *
 * For real plugins, replace SpinningCube with your own scene components and
 * move them into ui/src/r3f/components/.
 */

function SpinningCube() {
  const meshRef = useRef<Mesh>(null);

  useFrame((_state, delta) => {
    if (meshRef.current) {
      meshRef.current.rotation.x += delta * 0.8;
      meshRef.current.rotation.y += delta * 0.5;
    }
  });

  return (
    <mesh ref={meshRef}>
      <boxGeometry args={[1.5, 1.5, 1.5]} />
      <meshStandardMaterial color="#7c3aed" metalness={0.3} roughness={0.4} />
    </mesh>
  );
}

export default function ExampleScene() {
  return (
    <ThreeCanvas height="300px">
      <ambientLight intensity={0.4} />
      <directionalLight position={[5, 5, 5]} intensity={1} />
      <SpinningCube />
    </ThreeCanvas>
  );
}
