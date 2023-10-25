import {
  Services,
  Systems,
  uint8ArrayToBuffer,
} from "./node_modules/connected-spaces-platform.web/connectedspacesplatform.js";

export const testImage =
  "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACsAAAAzCAIAAACBumZhAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAJcEhZcwAAJOgAACToAYJjBRwAAAB5SURBVFhH7dTBCcAgEERR67Ig67Eam7GYDQNexeyaRAL";

export const resultState = (resultObject) => {
  switch (resultObject.getResultCode()) {
    case Services.EResultCode.Success:
      return "Successful";
    case Services.EResultCode.Failed:
      return "Failed";
    default:
      return "Unknown";
  }
};

export const dataUrlToFile = async (dataUrl, fileName, type = "image/png") => {
  const res = await fetch(dataUrl);
  const blob = await res.blob();

  return new File([blob], fileName, { type });
};

export const fileToBuffer = async (file, type = "image/png") => {
  const buffer = Systems.BufferAssetDataSource.create();
  buffer.bufferLength = file.size;
  const arrayBuffer = await file.arrayBuffer();
  const rawData = new Uint8Array(arrayBuffer, 0, file.size);
  buffer.buffer = uint8ArrayToBuffer(rawData);
  buffer.setMimeType(type);

  return buffer;
};

export function commonArrayToJSArray(array) {
  let _array = [];

  for (let i = 0; i < array.size(); i++) _array.push(array.get(i));

  return _array;
}
