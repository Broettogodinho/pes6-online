// Inicialização do WebGPU num <canvas>. Na PoC só prova que o dispositivo
// sobe e consegue apresentar um quadro; a tradução de chamadas gráficas
// (ex.: Direct3D 9 -> WebGPU) é etapa posterior.

export function webgpuAvailable() {
  return Boolean(globalThis.navigator?.gpu);
}

/**
 * @param {HTMLCanvasElement} canvas
 * @returns {Promise<{ device: GPUDevice, context: GPUCanvasContext, format: GPUTextureFormat, clear(r: number, g: number, b: number): void }>}
 */
export async function initWebGPU(canvas) {
  if (!webgpuAvailable()) throw new Error("WebGPU indisponível neste navegador");
  const adapter = await navigator.gpu.requestAdapter();
  if (!adapter) throw new Error("nenhum adaptador WebGPU encontrado");
  const device = await adapter.requestDevice();
  const context = canvas.getContext("webgpu");
  const format = navigator.gpu.getPreferredCanvasFormat();
  context.configure({ device, format, alphaMode: "opaque" });

  function clear(r, g, b) {
    const encoder = device.createCommandEncoder();
    const pass = encoder.beginRenderPass({
      colorAttachments: [{
        view: context.getCurrentTexture().createView(),
        clearValue: { r, g, b, a: 1 },
        loadOp: "clear",
        storeOp: "store",
      }],
    });
    pass.end();
    device.queue.submit([encoder.finish()]);
  }

  return { device, context, format, clear };
}
