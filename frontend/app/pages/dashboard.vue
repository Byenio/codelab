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

// Fetch Data (Directories + Repositories)
const url = computed(() => currentFolderId.value
    ? `/api/v1/directories?parent_id=${currentFolderId.value}`
    : '/api/v1/directories'
)

const { data, refresh, error } = await useApi<any>(url, {
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
      body: {
        name: newFolderName.value,
        parent_id: currentFolderId.value
      }
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

// Navigation Helpers
const goUp = () => {
  if (!data.value?.current_dir) return
  const parentId = data.value.current_dir.parent_id
  if (parentId) {
    router.push({ query: { folder: parentId } })
  } else {
    router.push({ query: {} }) // Go to root
  }
}

const goToFolder = (id: number) => {
  router.push({ query: { folder: id } })
}

const formatDate = (dateStr: string) => new Date(dateStr).toLocaleDateString()
</script>

<template>
  <div class="min-h-screen bg-zinc-50 dark:bg-zinc-950">
    <!-- Navbar (Simplified) -->
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

      <!-- Toolbar & Breadcrumbs -->
      <div class="flex flex-col md:flex-row justify-between items-start md:items-center gap-4 mb-6">
        <div class="flex items-center gap-2 text-lg">
          <UButton
              v-if="currentFolderId"
              icon="i-heroicons-arrow-left"
              variant="ghost"
              color="neutral"
              class="mr-1"
              @click="goUp"
          />
          <UIcon name="i-heroicons-home" class="text-zinc-400" />
          <span class="text-zinc-400">/</span>
          <span v-if="!currentFolderId" class="font-bold">Home</span>
          <template v-else>
            <span class="font-bold">{{ data?.current_dir?.name || '...' }}</span>
          </template>
        </div>

        <div class="flex gap-2">
          <!-- Modified: UModal wraps the trigger button -->
          <UModal v-model:open="isCreateFolderOpen">
            <UButton icon="i-heroicons-folder-plus" color="white" label="New Folder" />

            <template #content>
              <UCard>
                <template #header>
                  <h3 class="font-bold">Create New Folder</h3>
                </template>
                <form @submit.prevent="createFolder">
                  <UInput v-model="newFolderName" placeholder="Folder Name" autofocus class="mb-4" />
                  <div class="flex justify-end gap-2">
                    <UButton color="neutral" variant="ghost" @click="isCreateFolderOpen = false">Cancel</UButton>
                    <UButton type="submit" :loading="isCreating" :disabled="!newFolderName">Create</UButton>
                  </div>
                </form>
              </UCard>
            </template>
          </UModal>

          <!-- Pass current folder to New Repo page -->
          <UButton
              :to="currentFolderId ? `/new?folder=${currentFolderId}` : '/new'"
              icon="i-heroicons-plus"
              color="primary"
          >
            New Repository
          </UButton>
        </div>
      </div>

      <!-- Content Grid -->
      <div v-if="data" class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-4">

        <!-- Folders -->
        <div
            v-for="dir in data.directories"
            :key="'d'+dir.id"
            @click="goToFolder(dir.id)"
            class="bg-white dark:bg-zinc-900 p-4 rounded-lg border border-zinc-200 dark:border-zinc-800 hover:border-blue-400 cursor-pointer flex items-center gap-3 transition-colors"
        >
          <UIcon name="i-heroicons-folder" class="w-8 h-8 text-blue-400" />
          <span class="font-medium truncate">{{ dir.name }}</span>
        </div>

        <!-- Repositories -->
        <NuxtLink
            v-for="repo in data.repositories"
            :key="'r'+repo.id"
            :to="`/${repo.name}`"
            class="block"
        >
          <div class="bg-white dark:bg-zinc-900 p-4 rounded-lg border border-zinc-200 dark:border-zinc-800 hover:border-primary-500 h-full flex flex-col transition-colors">
            <div class="flex items-start justify-between mb-2">
              <div class="flex items-center gap-2">
                <UIcon name="i-heroicons-book-open" class="text-zinc-500" />
                <span class="font-bold truncate">{{ repo.name }}</span>
              </div>
              <UIcon v-if="repo.is_private" name="i-heroicons-lock-closed" class="w-4 h-4 text-zinc-400" />
            </div>
            <p class="text-xs text-zinc-500 line-clamp-2">{{ repo.description || 'No description' }}</p>
          </div>
        </NuxtLink>

      </div>

      <!-- Empty State -->
      <div v-if="data && data.directories.length === 0 && data.repositories.length === 0" class="text-center py-12 border-2 border-dashed border-zinc-200 dark:border-zinc-800 rounded-xl">
        <UIcon name="i-heroicons-folder-open" class="w-12 h-12 text-zinc-300 mx-auto mb-2" />
        <p class="text-zinc-500">This folder is empty.</p>
      </div>

    </main>
  </div>
</template>