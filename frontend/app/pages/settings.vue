<script setup lang="ts">
import auth from "~/middleware/auth";

definePageMeta({
  middleware: auth
})
</script>

<template>
  <div class="min-h-screen bg-zinc-50 dark:bg-zinc-950">
    <main class="max-w-7xl mx-auto p-6 flex gap-8">
      <!-- Sidebar -->
      <aside class="w-64 flex-shrink-0">
        <nav class="space-y-1">
          <NuxtLink
              to="/settings/keys"
              class="flex items-center gap-2 px-3 py-2 text-sm font-medium rounded-md bg-white dark:bg-zinc-900 border border-zinc-200 dark:border-zinc-800 text-neutral-100"
          >
            <UIcon name="i-heroicons-key" />
            SSH Keys
          </NuxtLink>
          <!-- Add Profile/Password later -->
        </nav>
      </aside>

      <!-- Content -->
      <div class="flex-1">
        <NuxtPage />
      </div>
    </main>
  </div>
</template>