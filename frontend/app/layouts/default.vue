<script setup lang="ts">
import {navigateTo, useCookie } from "nuxt/app"

const handleLogout = async () => {
  const cookie = useCookie('auth_token')
  cookie.value = null
  await navigateTo('/login')
}
</script>

<template>
  <div class="min-h-screen bg-zinc-50 dark:bg-zinc-950">
    <!-- Navbar -->
    <header class="border-b border-zinc-200 dark:border-zinc-800 bg-white dark:bg-zinc-900">
      <div class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 h-16 flex items-center justify-between">
        <div class="flex items-center gap-4">
          <NuxtLink to="/dashboard" class="text-xl font-bold text-primary-500">CodeLab</NuxtLink>
          <nav class="flex gap-4 text-sm font-medium">
            <NuxtLink to="/dashboard" class="text-zinc-900 dark:text-white hover:text-primary-500">Dashboard</NuxtLink>
            <NuxtLink to="/settings" class="text-zinc-500 hover:text-zinc-900 dark:hover:text-white">Settings</NuxtLink>
          </nav>
        </div>

        <UButton
            @click="handleLogout"
            variant="ghost"
            icon="i-heroicons-arrow-right-on-rectangle"
        >
          Logout
        </UButton>
      </div>
    </header>

    <!-- Page Content -->
    <slot />
  </div>
</template>