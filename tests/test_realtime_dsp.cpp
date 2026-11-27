#include <algorithm>
#include "engine/graph.h"
#include "dsp/effects.h"

#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <thread>
#if defined(_WIN32)
#include <malloc.h>
#endif

namespace {
void* allocateAligned(std::size_t n, std::size_t alignment) {
#if defined(_WIN32)
  return _aligned_malloc(n ? n : 1, alignment);
#else
  void* p = nullptr;
  return posix_memalign(&p, alignment, n ? n : 1) == 0 ? p : nullptr;
#endif
}
void freeAligned(void* p) noexcept {
#if defined(_WIN32)
  _aligned_free(p);
#else
  std::free(p);
#endif
}
thread_local bool countAllocations = false;
std::atomic<std::size_t> allocationCount{0};

class TestTone final : public wezt::Processor {
 public:
  void prepare(double sr, std::size_t) override { increment_ = 2.0 * 3.14159265358979323846 * 110.0 / sr; phase_ = 0.0; }
  void process(const wezt::AudioBuffer&, wezt::AudioBuffer& out) override {
    for (std::size_t i = 0; i < out.frames; ++i) {
      const float s = static_cast<float>(0.18 * std::sin(phase_));
      phase_ += increment_;
      for (std::size_t c = 0; c < out.channels; ++c) out.channel(c)[i] = s;
    }
  }
 private:
  double increment_ = 0.0, phase_ = 0.0;
};
}

void* operator new(std::size_t n) {
  if (countAllocations) allocationCount.fetch_add(1, std::memory_order_relaxed);
  if (void* p = std::malloc(n ? n : 1)) return p;
  throw std::bad_alloc();
}
void* operator new[](std::size_t n) {
  if (countAllocations) allocationCount.fetch_add(1, std::memory_order_relaxed);
  if (void* p = std::malloc(n ? n : 1)) return p;
  throw std::bad_alloc();
}
void* operator new(std::size_t n, std::align_val_t alignment) {
  if (countAllocations) allocationCount.fetch_add(1, std::memory_order_relaxed);
  if (void* p = allocateAligned(n, static_cast<std::size_t>(alignment))) return p;
  throw std::bad_alloc();
}
void* operator new[](std::size_t n, std::align_val_t alignment) {
  if (countAllocations) allocationCount.fetch_add(1, std::memory_order_relaxed);
  if (void* p = allocateAligned(n, static_cast<std::size_t>(alignment))) return p;
  throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
void operator delete(void* p, std::align_val_t) noexcept { freeAligned(p); }
void operator delete[](void* p, std::align_val_t) noexcept { freeAligned(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { freeAligned(p); }
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { freeAligned(p); }

int main() {
  using namespace wezt;
  constexpr double sampleRate = 48000.0;
  constexpr std::size_t maxFrames = 1024;
  Graph graph;
  const auto source = graph.add(std::make_unique<TestTone>());
  auto equalizer = std::make_unique<dsp::ParametricEq>();
  auto* equalizerControl = equalizer.get();
  const auto eq = graph.add(std::move(equalizer));
  auto compressor = std::make_unique<dsp::Compressor>();
  auto* compressorControl = compressor.get();
  const auto comp = graph.add(std::move(compressor));
  const auto sat = graph.add(std::make_unique<dsp::Saturator>());
  const auto deesser = graph.add(std::make_unique<dsp::DeEsser>());
  const auto delay = graph.add(std::make_unique<dsp::StereoDelay>());
  const auto reverb = graph.add(std::make_unique<dsp::Reverb>());
  const auto width = graph.add(std::make_unique<dsp::StereoWidth>());
  auto limiterProcessor = std::make_unique<dsp::Limiter>();
  auto* limiterControl = limiterProcessor.get();
  const auto limiter = graph.add(std::move(limiterProcessor));
  const std::array<Graph::NodeId, 8> chain{eq, comp, sat, deesser, delay, reverb, width, limiter};
  Graph::NodeId previous = source;
  for (auto node : chain) {
    if (!graph.connect(previous, node)) { std::puts("FAIL: graph connection"); return 1; }
    previous = node;
  }
  graph.setOutput(limiter);
  graph.prepare(sampleRate, 2, maxFrames);
  AudioBuffer output(2, maxFrames);
  constexpr std::array<std::size_t, 6> blocks{64, 128, 256, 512, 1024, 96};
  std::array<double, 6000> elapsedUs{};
  std::size_t elapsedCount = 0;
  allocationCount.store(0, std::memory_order_relaxed);
  std::atomic<bool> controlDone{false};
  std::thread control([&] {
    for (int i = 0; i < 20000; ++i) {
      const float v = static_cast<float>(i % 100) / 100.f;
      compressorControl->set({-30.f + 20.f * v, 1.f + 8.f * v, 0.2f + 20.f * v,
                               50.f + 500.f * v, 3.f, 1.f * v, 0.8f});
      limiterControl->set({-0.5f - 5.f * v, 40.f + 300.f * v, 3.f});
      const dsp::EqBand bands[2] = {{dsp::FilterType::LowShelf, 90.f + 100.f * v, -3.f * v, .707f, true},
                                    {dsp::FilterType::HighShelf, 7000.f, 2.f * v, .707f, true}};
      (void)equalizerControl->trySetBands(bands, 2);
    }
    controlDone.store(true, std::memory_order_release);
  });
  countAllocations = true;
  for (std::size_t i = 0; i < elapsedUs.size(); ++i) {
    const auto frames = blocks[i % blocks.size()];
    const auto start = std::chrono::steady_clock::now();
    graph.render(output, frames);
    const auto stop = std::chrono::steady_clock::now();
    elapsedUs[elapsedCount++] = std::chrono::duration<double, std::micro>(stop - start).count();
    if (output.frames != frames) { countAllocations = false; control.join(); std::puts("FAIL: render frame count"); return 1; }
    for (std::size_t c = 0; c < output.channels; ++c)
      for (std::size_t f = 0; f < frames; ++f)
        if (!std::isfinite(output.channel(c)[f])) { countAllocations = false; control.join(); std::puts("FAIL: non-finite audio"); return 1; }
  }
  countAllocations = false;
  control.join();
  const auto allocations = allocationCount.load(std::memory_order_relaxed);
  double total = 0.0, maximum = 0.0;
  for (double us : elapsedUs) { total += us; if (us > maximum) maximum = us; }
  if (allocations != 0) {
    std::printf("FAIL: %zu heap allocations during %zu real-time graph callbacks\n", allocations, elapsedCount);
    return 1;
  }
  std::sort(elapsedUs.begin(), elapsedUs.end());
  const double p99 = elapsedUs[static_cast<std::size_t>(elapsedUs.size() * 0.99)];
  std::printf("PASS: %zu callbacks, zero audio-thread heap allocations, all samples finite, concurrent parameter updates completed=%s; avg %.2f us, p99 %.2f us, max %.2f us\n",
              elapsedCount, controlDone.load(std::memory_order_acquire) ? "yes" : "no",
              total / static_cast<double>(elapsedCount), p99, maximum);
  return 0;
}
