<script setup lang="ts">
import auth from "~~/middleware/auth";
import type { Repository } from "~~/types/models";
import { useApi } from "~~/composables/useApi";

definePageMeta({
  middleware: auth
})

const { data: repos, error } = await useApi<Repository[]>('/api/v1/user/repositories')
const formatDate = (dateStr: string) => {
  return new Date(dateStr).toLocaleDateString()
}
</script>

<template>
  <div class="min-h-screen bg-zinc-50 dark:bg-zinc-950">
    <!-- Top Navigation Bar -->
    <header class="border-b border-zinc-200 dark:border-zinc-800 bg-white dark:bg-zinc-900">
      <div class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div class="flex justify-between h-16 items-center">
          <div class="flex items-center gap-4">
            <h1 class="text-xl font-bold text-primary-500">CodeLab</h1>
            <nav class="hidden md:flex gap-4 text-sm font-medium text-zinc-600 dark:text-zinc-300">
              <NuxtLink to="/dashboard" class="hover:text-primary-500">Dashboard</NuxtLink>
              <NuxtLink to="/settings" class="hover:text-primary-500">Settings</NuxtLink>
            </nav>
          </div>
          <div class="flex items-center gap-4">
            <UButton
                to="/new"
                icon="i-heroicons-plus"
                color="primary"
                variant="solid"
            >
              New Repository
            </UButton>
            <UButton
                to="/login"
                icon="i-heroicons-arrow-right-on-rectangle"
                color="primary"
                variant="ghost"
                @click="useCookie('auth_token').value = null"
            >
              Logout
            </UButton>
          </div>
        </div>
      </div>
    </header>

    <!-- Main Content -->
    <main class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-8">

      <div class="flex justify-between items-center mb-6">
        <h2 class="text-2xl font-bold text-zinc-900 dark:text-white">Your Repositories</h2>
      </div>

      <!-- Loading State -->
      <div v-if="!repos && !error" class="space-y-4">
        <USkeleton class="h-24 w-full" v-for="i in 3" :key="i" />
      </div>

      <!-- Error State -->
      <UAlert
          v-if="error"
          icon="i-heroicons-exclamation-triangle"
          color="error"
          variant="subtle"
          title="Failed to load repositories"
          :description="error.message"
      />

      <!-- Empty State -->
      <div v-if="repos && repos.length === 0" class="text-center py-12">
        <UIcon name="i-heroicons-cube-transparent" class="w-16 h-16 mx-auto text-zinc-400 mb-4" />
        <h3 class="text-lg font-medium text-zinc-900 dark:text-white">No repositories yet</h3>
        <p class="mt-1 text-zinc-500">Get started by creating your first project.</p>
        <UButton to="/new" class="mt-6" icon="i-heroicons-plus">Create Repository</UButton>
      </div>

      <!-- Repo Grid -->
      <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
        <NuxtLink
            v-for="repo in repos"
            :key="repo.id"
            :to="`/${repo.name}`"
            class="block"
        >
          <UCard class="h-full hover:border-primary-500 transition-colors cursor-pointer">
            <template #header>
              <div class="flex justify-between items-start">
                <div class="flex items-center gap-2">
                  <UIcon name="i-heroicons-book-open" class="text-zinc-500" />
                  <span class="font-semibold text-lg hover:text-primary-500 hover:underline">
                    {{ repo.name }}
                  </span>
                  <UBadge v-if="repo.is_private" color="primary" size="xs" variant="solid">Private</UBadge>
                  <UBadge v-else color="success" size="xs" variant="subtle">Public</UBadge>
                </div>
              </div>
            </template>

            <p class="text-sm text-zinc-500 dark:text-zinc-400 line-clamp-2 h-10">
              {{ repo.description || 'No description provided.' }}
            </p>

            <template #footer>
              <div class="flex items-center gap-2 text-xs text-zinc-400">
                <span class="w-2 h-2 rounded-full bg-yellow-400"></span>
                <span>C++</span> <!-- TODO: language detection -->
                <span class="ml-auto">Updated {{ formatDate(repo.created_at) }}</span>
              </div>
            </template>
          </UCard>
        </NuxtLink>
      </div>
    </main>
  </div>
</template>