<script setup lang="ts">
import auth from "~~/middleware/auth";
import { useApi } from "~~/composables/useApi";

definePageMeta({
  middleware: auth
})

const route = useRoute()
const router = useRouter()

// Current Folder State
const currentFolderId = computed(() => {
  const id = route.query.folder
  return id ? parseInt(id as string) : null
})

// Fetch Data
const url = computed(() => currentFolderId.value
    ? `/api/v1/directories?parent_id=${currentFolderId.value}`
    : '/api/v1/directories'
)

const { data, refresh } = await useApi<any>(url, {
  watch: [currentFolderId]
})

// Create Folder State
const isCreateFolderOpen = ref(false)
const newFolderName = ref('')
const isCreating = ref(false)

async function createFolder() {
  if (!newFolderName.value) return
  isCreating.value = true
  try {
    await useApi('/api/v1/directories', {
      method: 'POST',
      body: { name: newFolderName.value, parent_id: currentFolderId.value }
    })
    isCreateFolderOpen.value = false
    newFolderName.value = ''
    refresh()
  } catch (e) {
    // handle error
  } finally {
    isCreating.value = false
  }
}

// Navigation
const goUp = () => {
  if (!data.value?.current_dir) return
  const parentId = data.value.current_dir.parent_id
  if (parentId) router.push({ query: { folder: parentId } })
  else router.push({ query: {} })
}

const goToFolder = (id: number) => {
  router.push({ query: { folder: id } })
}
</script>

<template>
  <div class="min-h-screen bg-zinc-50 dark:bg-zinc-950">
    <!-- Navbar -->
    <header class="border-b border-zinc-200 dark:border-zinc-800 bg-white dark:bg-zinc-900">
      <div class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 h-16 flex items-center justify-between">
        <div class="flex items-center gap-4">
          <h1 class="text-xl font-bold text-primary-500">CodeLab</h1>
          <nav class="flex gap-4 text-sm font-medium">
            <NuxtLink to="/dashboard" class="text-zinc-900 dark:text-white">Dashboard</NuxtLink>
            <NuxtLink to="/settings/keys" class="text-zinc-500 hover:text-zinc-900">Settings</NuxtLink>
          </nav>
        </div>
        <UButton to="/login" variant="ghost" icon="i-heroicons-arrow-right-on-rectangle">Logout</UButton>
      </div>
    </header>

    <main class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-8">

      <!-- Toolbar -->
      <div class="flex flex-col md:flex-row justify-between items-start md:items-center gap-4 mb-6">
        <div class="flex items-center gap-2 text-lg">
          <UButton v-if="currentFolderId" icon="i-heroicons-arrow-left" variant="ghost" color="neutral" class="mr-1" @click="goUp"/>
          <UIcon name="i-heroicons-home" class="text-zinc-400" />
          <span class="text-zinc-400">/</span>
          <span v-if="!currentFolderId" class="font-bold">Home</span>
          <template v-else>
            <span class="font-bold">{{ data?.current_dir?.name || '...' }}</span>
          </template>
        </div>

        <div class="flex gap-2">
          <!-- Button triggers the modal -->
          <UButton
              icon="i-heroicons-folder-plus"
              color="neutral"
              label="New Folder"
              @click="isCreateFolderOpen = true"
          />

          <UButton
              :to="currentFolderId ? `/new?folder=${currentFolderId}` : '/new'"
              icon="i-heroicons-plus"
              color="primary"
          >
            New Repository
          </UButton>
        </div>
      </div>

      <!-- LIST VIEW -->
      <UCard v-if="data" class="overflow-hidden p-0">
        <div class="divide-y divide-zinc-100 dark:divide-zinc-800">

          <!-- Empty State -->
          <div v-if="data.directories.length === 0 && data.repositories.length === 0" class="p-8 text-center text-zinc-500">
            This folder is empty.
          </div>

          <!-- Directories -->
          <div
              v-for="dir in data.directories"
              :key="'d'+dir.id"
              @click="goToFolder(dir.id)"
              class="flex items-center justify-between p-3 px-4 hover:bg-zinc-50 dark:hover:bg-zinc-800/50 cursor-pointer group transition-colors"
          >
            <div class="flex items-center gap-3">
              <UIcon name="i-heroicons-folder" class="w-5 h-5 text-blue-400" />
              <span class="text-sm font-medium text-zinc-700 dark:text-zinc-200 group-hover:text-primary-500 group-hover:underline">
                {{ dir.name }}
              </span>
            </div>
            <UIcon name="i-heroicons-chevron-right" class="w-4 h-4 text-zinc-300" />
          </div>

          <!-- Repositories -->
          <NuxtLink
              v-for="repo in data.repositories"
              :key="'r'+repo.id"
              :to="`/${repo.name}`"
              class="flex items-center justify-between p-3 px-4 hover:bg-zinc-50 dark:hover:bg-zinc-800/50 cursor-pointer group transition-colors"
          >
            <div class="flex items-center gap-3">
              <UIcon name="i-heroicons-book-open" class="w-5 h-5 text-zinc-400" />
              <div class="flex flex-col">
                <div class="flex items-center gap-2">
                  <span class="text-sm font-bold text-zinc-700 dark:text-zinc-200 group-hover:text-primary-500 group-hover:underline">
                    {{ repo.name }}
                  </span>
                  <UIcon v-if="repo.is_private" name="i-heroicons-lock-closed" class="w-3 h-3 text-zinc-400" />
                </div>
                <span v-if="repo.description" class="text-xs text-zinc-400 line-clamp-1">{{ repo.description }}</span>
              </div>
            </div>
            <div class="flex items-center gap-4 text-xs text-zinc-400">
              <span>Updated recently</span>
            </div>
          </NuxtLink>

        </div>
      </UCard>
    </main>

    <!-- Modal Component (Nuxt UI v3 Syntax: v-model:open) -->
    <UModal v-model:open="isCreateFolderOpen" title="Create New Folder">
      <template #body>
        <form @submit.prevent="createFolder" class="space-y-4">
          <UFormField label="Folder Name">
            <UInput v-model="newFolderName" placeholder="e.g. My Projects" autofocus />
          </UFormField>

          <div class="flex justify-end gap-2 pt-2">
            <UButton color="neutral" variant="ghost" @click="isCreateFolderOpen = false">Cancel</UButton>
            <UButton type="submit" :loading="isCreating" :disabled="!newFolderName">Create</UButton>
          </div>
        </form>
      </template>
    </UModal>
  </div>
</template>