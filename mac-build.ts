// build_script.ts

import { Command } from "https://deno.land/x/cliffy@v1.0.0-rc.4/command/mod.ts";
import { copy } from "https://deno.land/std/fs/copy.ts";
import { join } from "https://deno.land/std/path/mod.ts";
import { exists } from "https://deno.land/std/fs/exists.ts";
import { emptyDir } from "https://deno.land/std/fs/empty_dir.ts";

async function runCommand(cmd: string[], cwd?: string): Promise<boolean> {
  try {
    const process = new Deno.Command(cmd[0], {
      args: cmd.slice(1),
      cwd: cwd,
      stdout: "inherit",
      stderr: "inherit",
    });
    
    const { success } = await process.output();
    return success;
  } catch (error) {
    console.error(`Error executing command: ${cmd.join(" ")}`);
    console.error(error);
    return false;
  }
}

async function copyDirectory(from: string, to: string): Promise<boolean> {
  try {
    await copy(from, to, { overwrite: true });
    return true;
  } catch (error) {
    console.error(`Error copying from ${from} to ${to}`);
    console.error(error);
    return false;
  }
}

async function verifyWasmFiles(buildType: string): Promise<boolean> {
  const wasmPath = join(".", "Library", "Binaries", "wasm", buildType);
  const requiredFiles = ["ConnectedSpacesPlatform_WASM.wasm"];
  
  for (const file of requiredFiles) {
    if (!await exists(join(wasmPath, file))) {
      return false;
    }
  }
  return true;
}

type BuildOptions = {
  skipGenerate?: boolean;
  noConfigure?: boolean;
};

async function buildProcess(buildType: string, options: BuildOptions = {}) {
  const buildTypeCapitalized = buildType.charAt(0).toUpperCase() + buildType.slice(1);
  let step = 1;
  const totalSteps = options.skipGenerate ? 4 : 5;
  
  // Clean up existing wasm files
  const wasmPath = join(".", "Library", "Binaries", "wasm", buildTypeCapitalized);
  console.log(`\nCleaning up existing wasm files in ${wasmPath}...`);
  try {
    await emptyDir(wasmPath);
  } catch (error) {
    console.error(`Error cleaning wasm directory: ${error}`);
    throw new Error("Failed to clean wasm directory");
  }
  
  // Step 1: Run generate solution script (skip if update command)
  if (!options.skipGenerate) {
    console.log(`\n[${step}/${totalSteps}] Running generate solution script for ${buildType}...`);
    if (!await runCommand(["sh", "./generate_solution_mac.sh", buildType])) {
      throw new Error("Failed to generate solution");
    }
    step++;
  }

  // Step 2: Run Emscripten build script
  console.log(`\n[${step}/${totalSteps}] Running Emscripten build script for ${buildType}...`);
  const emscriptenScript = options.noConfigure ? 
    "./EmscriptenFullBuildNoConfigure.sh" : 
    "./EmscriptenFullBuildAndConfigure.sh";
  
  if (!await runCommand(
    ["sh", emscriptenScript, buildType],
    "./Tools/Emscripten"
  )) {
    throw new Error("Failed to run Emscripten build");
  }
  step++;

    // Step 3 Verify WASM files exist before copying
  console.log("\nVerifying WASM files were generated...");
  if (!await verifyWasmFiles(buildTypeCapitalized)) {
    throw new Error("Build Failed: WASM files were not generated successfully");
  }


  // Step 4: Run wrapper generator
  console.log(`\n[${step}/${totalSteps}] Running wrapper generator...`);
  if (!await runCommand([
    "python3",
    "Tools/WrapperGenerator/WrapperGenerator.py",
    "--generate_typescript"
  ])) {
    throw new Error("Failed to run wrapper generator");
  }
  step++;


  // Step 5: Copy binary files
  const binarySourcePath = join(".", "Library", "Binaries", "wasm", buildTypeCapitalized);
  const binaryDestPath = join("..", "oko-web", "node_modules", "connected-spaces-platform.web", buildTypeCapitalized);
  
  console.log(`\n[${step}/${totalSteps}] Copying binary files from ${binarySourcePath} to ${binaryDestPath}...`);
  if (!await copyDirectory(binarySourcePath, binaryDestPath)) {
    throw new Error("Failed to copy binary files");
  }
  step++;

  // Step 6: Copy TypeScript files
  const tsSourcePath = join(".", "Tools", "WrapperGenerator", "Output", "TypeScript");
  const tsDestPath = join("..", "oko-web", "node_modules", "connected-spaces-platform.web");
  
  console.log(`\n[${step}/${totalSteps}] Copying TypeScript files from ${tsSourcePath} to ${tsDestPath}...`);
  if (!await copyDirectory(tsSourcePath, tsDestPath)) {
    throw new Error("Failed to copy TypeScript files");
  }

  console.log("\nBuild process completed successfully! 🎉");
}

const buildTypeOption = {
  type: "string" as const,
  description: "Build type (debug or release)",
  default: "debug",
  validate: (value: string) => {
    if (!["debug", "release"].includes(value.toLowerCase())) {
      throw new Error("Build type must be either 'debug' or 'release'");
    }
    return value.toLowerCase();
  }
};

await new Command()
  .name("build-script")
  .version("1.0.0")
  .description("Build script for managing the build process")
  .command("init", "Initialize and run the full build process")
  .option("-t, --type <type>", "build type", buildTypeOption)
  .action(async ({ type }) => {
    try {
      await buildProcess(type);
    } catch (error: unknown) {
      if (error instanceof Error) {
        console.error("\n❌ Error:", error.message);
      } else {
        console.error("\n❌ Error:", String(error));
      }
      Deno.exit(1);
    }
  })
  .command("update", "Run an update build (skips generate solution and uses NoConfigure)")
  .option("-t, --type <type>", "Build Type", buildTypeOption)
  .action(async ({ type }) => {
    try {
      await buildProcess(type, { 
        skipGenerate: true, 
        noConfigure: true 
      });
    } catch (error: unknown) {
      if (error instanceof Error) {
        console.error("\n❌ Error:", error.message);
      } else {
        console.error("\n❌ Error:", String(error));
      }
      Deno.exit(1);
    }
  })
  .parse(Deno.args);