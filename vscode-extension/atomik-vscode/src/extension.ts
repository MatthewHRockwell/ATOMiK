import * as vscode from "vscode";
import { execFile } from "child_process";

const OUTPUT_CHANNEL_NAME = "ATOMiK";
let outputChannel: vscode.OutputChannel;

function getOutputChannel(): vscode.OutputChannel {
  if (!outputChannel) {
    outputChannel = vscode.window.createOutputChannel(OUTPUT_CHANNEL_NAME);
  }
  return outputChannel;
}

function runAtomikGen(
  args: string[],
  cwd?: string
): Promise<{ stdout: string; stderr: string }> {
  return new Promise((resolve, reject) => {
    execFile("atomik-gen", args, { cwd }, (error, stdout, stderr) => {
      if (error && !stdout && !stderr) {
        reject(
          new Error(
            `Failed to run atomik-gen: ${error.message}. ` +
              "Ensure the atomik-sdk Python package is installed (pip install -e ./software)."
          )
        );
        return;
      }
      resolve({ stdout: stdout || "", stderr: stderr || "" });
    });
  });
}

function getActiveFilePath(): string | undefined {
  const editor = vscode.window.activeTextEditor;
  if (!editor) {
    vscode.window.showWarningMessage("No active editor. Open a schema JSON file first.");
    return undefined;
  }
  const filePath = editor.document.uri.fsPath;
  if (!filePath.endsWith(".json")) {
    vscode.window.showWarningMessage("Active file is not a JSON file.");
    return undefined;
  }
  return filePath;
}

async function cmdGenerate(): Promise<void> {
  const filePath = getActiveFilePath();
  if (!filePath) {
    return;
  }

  const outputDir = await vscode.window.showInputBox({
    prompt: "Output directory for generated code",
    value: "generated",
  });
  if (outputDir === undefined) {
    return;
  }

  const channel = getOutputChannel();
  channel.show(true);
  channel.appendLine(`Generating SDK from: ${filePath}`);
  channel.appendLine(`Output directory: ${outputDir}`);
  channel.appendLine("");

  try {
    const { stdout, stderr } = await runAtomikGen(
      ["generate", filePath, "--output-dir", outputDir, "--verbose"]
    );
    if (stdout) {
      channel.appendLine(stdout);
    }
    if (stderr) {
      channel.appendLine(stderr);
    }
    vscode.window.showInformationMessage("ATOMiK: SDK generation complete.");
  } catch (err: unknown) {
    const message = err instanceof Error ? err.message : String(err);
    channel.appendLine(`Error: ${message}`);
    vscode.window.showErrorMessage(`ATOMiK generation failed: ${message}`);
  }
}

async function cmdValidate(): Promise<void> {
  const filePath = getActiveFilePath();
  if (!filePath) {
    return;
  }

  const channel = getOutputChannel();
  channel.show(true);
  channel.appendLine(`Validating: ${filePath}`);
  channel.appendLine("");

  try {
    const { stdout, stderr } = await runAtomikGen(["validate", filePath]);
    if (stdout) {
      channel.appendLine(stdout);
    }
    if (stderr) {
      channel.appendLine(stderr);
    }
    if (!stderr) {
      vscode.window.showInformationMessage("ATOMiK: Schema validation passed.");
    }
  } catch (err: unknown) {
    const message = err instanceof Error ? err.message : String(err);
    channel.appendLine(`Error: ${message}`);
    vscode.window.showErrorMessage(`ATOMiK validation failed: ${message}`);
  }
}

async function cmdBatch(): Promise<void> {
  const folders = await vscode.window.showOpenDialog({
    canSelectFolders: true,
    canSelectFiles: false,
    canSelectMany: false,
    openLabel: "Select schemas directory",
  });
  if (!folders || folders.length === 0) {
    return;
  }
  const schemasDir = folders[0].fsPath;

  const outputDir = await vscode.window.showInputBox({
    prompt: "Output directory for generated code",
    value: "generated",
  });
  if (outputDir === undefined) {
    return;
  }

  const channel = getOutputChannel();
  channel.show(true);
  channel.appendLine(`Batch generating from: ${schemasDir}`);
  channel.appendLine(`Output directory: ${outputDir}`);
  channel.appendLine("");

  try {
    const { stdout, stderr } = await runAtomikGen(
      ["batch", schemasDir, "--output-dir", outputDir, "--verbose"]
    );
    if (stdout) {
      channel.appendLine(stdout);
    }
    if (stderr) {
      channel.appendLine(stderr);
    }
    vscode.window.showInformationMessage("ATOMiK: Batch generation complete.");
  } catch (err: unknown) {
    const message = err instanceof Error ? err.message : String(err);
    channel.appendLine(`Error: ${message}`);
    vscode.window.showErrorMessage(`ATOMiK batch generation failed: ${message}`);
  }
}

async function cmdInfo(): Promise<void> {
  const filePath = getActiveFilePath();
  if (!filePath) {
    return;
  }

  const channel = getOutputChannel();
  channel.show(true);
  channel.appendLine(`Schema info: ${filePath}`);
  channel.appendLine("");

  try {
    const { stdout, stderr } = await runAtomikGen(["info", filePath]);
    if (stdout) {
      channel.appendLine(stdout);
    }
    if (stderr) {
      channel.appendLine(stderr);
    }
  } catch (err: unknown) {
    const message = err instanceof Error ? err.message : String(err);
    channel.appendLine(`Error: ${message}`);
    vscode.window.showErrorMessage(`ATOMiK info failed: ${message}`);
  }
}

export function activate(context: vscode.ExtensionContext): void {
  context.subscriptions.push(
    vscode.commands.registerCommand("atomik.generate", cmdGenerate),
    vscode.commands.registerCommand("atomik.validate", cmdValidate),
    vscode.commands.registerCommand("atomik.batch", cmdBatch),
    vscode.commands.registerCommand("atomik.info", cmdInfo)
  );
}

export function deactivate(): void {
  if (outputChannel) {
    outputChannel.dispose();
  }
}
