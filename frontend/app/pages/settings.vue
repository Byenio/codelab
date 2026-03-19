<script setup lang="ts">
import auth from "~~/middleware/auth";

definePageMeta({
  middleware: auth
})
</script>

<template>
  <div class="min-h-screen bg-zinc-50 dark:bg-zinc-950">
    <!-- Header -->
    <header class="bg-white dark:bg-zinc-900 border-b border-zinc-200 dark:border-zinc-800 p-4">
      <div class="max-w-7xl mx-auto flex items-center justify-between">
        <div class="flex items-center gap-4">
          <NuxtLink to="/dashboard" class="text-zinc-500 hover:text-primary-500">
            <UIcon name="i-heroicons-arrow-left" /> Dashboard
          </NuxtLink>
          <h1 class="text-xl font-bold">Account Settings</h1>
        </div>
      </div>
    </header>

    <main class="max-w-7xl mx-auto p-6 flex gap-8">
      <!-- Sidebar -->
      <aside class="w-64 flex-shrink-0">
        <nav class="space-y-1">
          <NuxtLink
              to="/settings/keys"
              class="flex items-center gap-2 px-3 py-2 text-sm font-medium rounded-md bg-white dark:bg-zinc-900 border border-zinc-200 dark:border-zinc-800 text-primary-600"
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