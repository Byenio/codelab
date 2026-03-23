<script setup lang="ts">
import { useApi } from "~/composables/useApi";
import authenticated from "~/middleware/authenticated";
// Import the new component
import RepositoryView from "~/components/RepositoryView.vue";

definePageMeta({
  middleware: authenticated
})

const route = useRoute()
const router = useRouter()

// Reconstruct path from slug array
const currentPath = computed(() => {
  const slug = route.params.slug
  if (!slug) return ''
  return Array.isArray(slug) ? slug.join('/') : slug
})

// Resolve the path (Backend returns either directory contents OR repository metadata)
const { data, error } = await useApi<any>('/api/v1/fs/resolve', {
  query: computed(() => ({ path: currentPath.value })),
  watch: [currentPath]
})

// --- Type Guards ---
const isDirectory = computed(() => !data.value || data.value.type === 'directory')
const isRepository = computed(() => data.value && data.value.type === 'repository')
const repoData = computed(() => data.value?.repository)
const repoDirId = computed(() => data.value?.directory_id)

// --- Navigation ---
const navigateToItem = (item: any) => {
  const newPath = currentPath.value ? `${currentPath.value}/${item.name}` : item.name
  router.push('/' + newPath)
}

// --- Breadcrumbs ---
const breadcrumbs = computed(() => {
  if (!currentPath.value) return []
  const segments = currentPath.value.split('/')
  const crumbs = []
  let accum = ''
  for (const seg of segments) {
    accum = accum ? `${accum}/${seg}` : seg
    crumbs.push({ name: seg, path: '/' + accum })
  }
  return crumbs
})
</script>

<template>
  <div class="min-h-screen bg-zinc-50 dark:bg-zinc-950">
    <main class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-8">

      <!-- Breadcrumbs Header -->
      <div class="flex items-center gap-2 text-sm text-zinc-500 mb-6">
        <NuxtLink to="/" class="hover:text-primary-500 flex items-center gap-1">
          <UIcon name="i-heroicons-home" class="w-4 h-4"/>
        </NuxtLink>
        <template v-for="crumb in breadcrumbs" :key="crumb.path">
          <span class="text-zinc-300">/</span>
          <NuxtLink :to="crumb.path" class="hover:text-primary-500 font-medium text-zinc-700 dark:text-zinc-200">
            {{ crumb.name }}
          </NuxtLink>
        </template>
      </div>

      <!-- ERROR STATE -->
      <div v-if="error" class="text-center py-12">
        <UIcon name="i-heroicons-exclamation-triangle" class="w-12 h-12 text-zinc-300 mx-auto mb-4" />
        <h3 class="text-lg font-medium text-zinc-900 dark:text-white">Path not found</h3>
        <p class="text-zinc-500 mt-2">The directory or repository you are looking for does not exist.</p>
        <UButton to="/" variant="ghost" class="mt-4">Go Home</UButton>
      </div>

      <!-- DIRECTORY VIEW -->
      <div v-else-if="isDirectory && data">
        <div class="flex justify-between items-center mb-6">
          <h1 class="text-2xl font-bold text-zinc-900 dark:text-white">
            {{ breadcrumbs.length ? breadcrumbs[breadcrumbs.length-1].name : 'Home' }}
          </h1>
          <div class="flex gap-2">
            <UButton icon="i-heroicons-folder-plus" color="neutral" label="New Folder" />
            <UButton icon="i-heroicons-plus" color="primary" label="New Repository" />
          </div>
        </div>

        <UCard class="p-0 overflow-hidden">
          <div class="divide-y divide-zinc-100 dark:divide-zinc-800">
            <div v-if="data.directories.length === 0 && data.repositories.length === 0" class="p-8 text-center text-zinc-500">
              Empty directory.
            </div>

            <!-- Directories -->
            <div
                v-for="dir in data.directories"
                :key="'d'+dir.id"
                @click="navigateToItem(dir)"
                class="flex items-center gap-3 p-3 px-4 hover:bg-zinc-50 dark:hover:bg-zinc-800/50 cursor-pointer group"
            >
              <UIcon name="i-heroicons-folder" class="w-5 h-5 text-primary-400" />
              <span class="text-sm font-medium text-zinc-700 dark:text-zinc-200 group-hover:text-primary-500">{{ dir.name }}</span>
            </div>

            <!-- Repositories -->
            <div
                v-for="repo in data.repositories"
                :key="'r'+repo.id"
                @click="navigateToItem(repo)"
                class="flex items-center gap-3 p-3 px-4 hover:bg-zinc-50 dark:hover:bg-zinc-800/50 cursor-pointer group"
            >
              <UIcon name="i-heroicons-book-open" class="w-5 h-5 text-zinc-400" />
              <span class="text-sm font-bold text-zinc-700 dark:text-zinc-200 group-hover:text-primary-500">{{ repo.name }}</span>
              <UIcon v-if="repo.is_private" name="i-heroicons-lock-closed" class="w-3 h-3 text-zinc-400" />
            </div>
          </div>
        </UCard>
      </div>

      <!-- REPOSITORY VIEW -->
      <!-- We render the component here if the type is repository -->
      <RepositoryView
          v-else-if="isRepository && repoData"
          :repo="repoData"
          :directory-id="repoDirId"
      />

    </main>
  </div>
</template>