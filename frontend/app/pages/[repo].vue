<script setup lang="ts">
import {useApi} from "~~/composables/useApi";
import {type FileEntry, type Repository} from "~~/types/models";

const route = useRoute()
const repoName = route.params.repo as string

const { data: files, error } = await useApi<FileEntry[]>(`/api/v1/repositories/${repoName}/tree`, {
  query: { branch: 'master' }
})

const { data: repo } = await useApi<Repository>(`/api/v1/repositories/${repoName}`)

const iconMap: Record<string, string> = {
  'README.md': 'i-heroicons-document-text',
  'default': 'i-heroicons-document',
  'folder': 'i-heroicons-folder'
}

const getFileIcon = (file: FileEntry) => {
  if (file.is_directory) return iconMap['folder'];
  return iconMap[file.name] || iconMap['default'];
}
</script>

<template>
  <div class="min-h-screen bg-zinc-50 dark:bg-zinc-950">
    <!-- Header with Breadcrumbs -->
    <header class="bg-white dark:bg-zinc-900 border-b border-zinc-200 dark:border-zinc-800 p-4">
      <div class="max-w-7xl mx-auto flex items-center gap-2 text-lg">
        <UIcon name="i-heroicons-book-open" class="text-zinc-500" />
        <span class="font-semibold text-primary-500">{{ repoName }}</span>
        <UBadge v-if="repo?.is_private" color="primary" size="xs">Private</UBadge>
      </div>
    </header>

    <main class="max-w-7xl mx-auto p-6">
      <UAlert v-if="error" color="error" variant="subtle" title="Error" :description="error.message" />

      <!-- File Browser -->
      <UCard class="mb-8">
        <div class="border-b border-zinc-100 dark:border-zinc-800 p-3 bg-zinc-50 dark:bg-zinc-900/50 rounded-t-lg flex justify-between">
          <div class="flex items-center gap-2">
            <UAvatar size="xs" src="https://github.com/ghost.png" />
            <span class="font-bold text-sm">Last Commit:</span>
            <span class="text-sm text-zinc-500 font-mono">Initial commit</span>
          </div>
          <span class="text-xs text-zinc-400">2 minutes ago</span>
        </div>

        <div class="divide-y divide-zinc-100 dark:divide-zinc-800">
          <div
              v-for="file in files"
              :key="file.path"
              class="flex items-center justify-between p-3 hover:bg-zinc-50 dark:hover:bg-zinc-800/50 cursor-pointer"
          >
            <div class="flex items-center gap-3">
              <UIcon
                  :name="getFileIcon(file)"
                  :class="file.is_directory ? 'text-blue-400' : 'text-zinc-400'"
                  class="w-5 h-5"
              />
              <span class="text-sm text-zinc-700 dark:text-zinc-200">{{ file.name }}</span>
            </div>
            <span class="text-xs text-zinc-400">3 days ago</span>
          </div>
        </div>
      </UCard>

      <!-- README Viewer -->
      <UCard>
        <template #header>
          <h3 class="font-bold flex items-center gap-2">
            <UIcon name="i-heroicons-bars-3-bottom-left" />
            README.md
          </h3>
        </template>
        <div class="prose dark:prose-invert max-w-none p-4">
          <h1>{{ repoName }}</h1>
          <p>Initialized with CodeLab.</p>
        </div>
      </UCard>
    </main>
  </div>
</template>