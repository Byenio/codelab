<script setup lang="ts">
const checkBackend = async () => {
  // This request goes to Nuxt -> Proxy -> C++ Backend (Port 8080)
  try {
    const data = await $fetch('/api/v1/health') // Assuming you have a health endpoint or similar
    console.log('Backend Response:', data)
    alert('Backend is connected!')
  } catch (error) {
    console.error(error)
    alert('Failed to connect to backend.')
  }
}
</script>

<template>
  <UContainer class="flex h-screen flex-col items-center justify-center gap-4">
    <h1 class="text-4xl font-bold text-primary-500">
      CodeLab
    </h1>
    <p class="text-gray-500 dark:text-gray-400">
      The open-source GitHub clone built with C++ and Nuxt UI.
    </p>

    <div class="flex gap-2">
      <UButton
          icon="i-heroicons-bolt"
          size="lg"
          color="primary"
          variant="solid"
          @click="checkBackend"
      >
        Check Connection
      </UButton>

      <UButton
          icon="i-heroicons-book-open"
          size="lg"
          color="gray"
          variant="ghost"
      >
        Documentation
      </UButton>
    </div>

    <!-- Example Card -->
    <UCard class="mt-8 w-full max-w-sm">
      <template #header>
        <div class="flex justify-between">
          <span class="font-bold">System Status</span>
          <UBadge color="green" variant="subtle">Online</UBadge>
        </div>
      </template>

      <div class="space-y-2">
        <div class="flex justify-between text-sm">
          <span>Backend</span>
          <span class="font-mono text-gray-500">C++ Crow</span>
        </div>
        <div class="flex justify-between text-sm">
          <span>Frontend</span>
          <span class="font-mono text-gray-500">Nuxt UI</span>
        </div>
      </div>
    </UCard>
  </UContainer>
</template>