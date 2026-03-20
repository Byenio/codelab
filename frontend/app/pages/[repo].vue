<script setup lang="ts">
import { useApi } from "~/composables/useApi";
import type { FileEntry, Repository } from "~~/types/models";

const route = useRoute()
const router = useRouter()

const repoName = computed(() => {
  const r = route.params.repo
  if (!r || r === 'null' || r === 'undefined') return ''
  return Array.isArray(r) ? r[0] : r as string
})

const currentDirectoryId = computed(() => (route.query.folder ? parseInt(route.query.folder as string) : null))
const currentBranch = computed(() => (route.query.branch as string) || 'master')
const currentPath = computed(() => (route.query.path as string) || '')
const viewType = computed(() => (route.query.type as string) || 'tree')

const apiQueryParams = computed(() => {
  const params: any = { branch: currentBranch.value, path: currentPath.value }
  if (currentDirectoryId.value) params.directory_id = currentDirectoryId.value

  return params
})

const repoUrl = computed(() => repoName.value ? `/api/v1/repositories/${repoName.value}` : null)
const { data: repo } = await useApi<Repository>(repoUrl, {
  query: computed(() => currentDirectoryId.value ? { directory_id: currentDirectoryId.value } : {}),
  watch: [repoUrl, currentDirectoryId]
})

const treeUrl = computed(() => {
  if (!repoName.value || viewType.value !== 'tree') return null
  return `/api/v1/repositories/${repoName.value}/tree`
})

const { data: files, pending: loadingFiles } = useApi<FileEntry[]>(
    treeUrl,
    {
      query: apiQueryParams,
      watch: [treeUrl, currentBranch, currentPath, currentDirectoryId],
      key: `tree-${route.fullPath}`
    }
)

const blobUrl = computed(() => {
  if (!repoName.value || viewType.value !== 'blob') return null
  return `/api/v1/repositories/${repoName.value}/blob`
})

const { data: fileContent, pending: loadingBlob } = useApi<{content: string}>(
    blobUrl,
    {
      query: apiQueryParams,
      watch: [blobUrl, currentBranch, currentPath, currentDirectoryId],
      key: `blob-${route.fullPath}`
    }
)

const hasReadme = computed(() => {
  if (!files.value) return null
  return files.value.find(f => f.name.toLowerCase() === 'readme.md')
})

const readmeUrl = computed(() => {
  if (viewType.value !== 'tree' || !hasReadme.value) return null
  return `/api/v1/repositories/${repoName.value}/blob`
})

const readmePath = computed(() => {
  if (!hasReadme.value) return ''
  return currentPath.value
      ? `${currentPath.value}/${hasReadme.value.name}`
      : hasReadme.value.name
})

const { data: readmeContent } = useApi<{content: string}>(
    readmeUrl,
    {
      query: computed(() => {
        const params: any = {
          branch: currentBranch.value,
          path: readmePath.value
        }
        if (currentDirectoryId.value) {
          params.directory_id = currentDirectoryId.value
        }
        return params
      }),
      watch: [readmeUrl, readmePath, currentBranch, currentDirectoryId]
    }
)

const navigateToItem = (item: FileEntry) => {
  const newPath = currentPath.value ? `${currentPath.value}/${item.name}` : item.name

  const query: any = { ...route.query, path: newPath, type: '' }

  if (item.is_directory) {
    query.type = 'tree'
  } else {
    query.type = 'blob'
  }

  router.push({
    name: 'repo',
    params: { repo: repoName.value },
    query
  })
}

const goUp = () => {
  if (!currentPath.value) return
  const segments = currentPath.value.split('/')
  segments.pop()
  const newPath = segments.join('/')

  const query = { ...route.query, path: newPath, type: 'tree' }

  router.push({
    name: 'repo',
    params: { repo: repoName.value },
    query
  })
}


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
  <div class="max-w-7xl mx-auto p-6">
    <!-- Repo Header (Breadcrumbs/Title) -->
    <div class="flex items-center gap-2 text-lg mb-6 border-b border-zinc-200 dark:border-zinc-800 pb-4">
      <UIcon name="i-heroicons-book-open" class="text-zinc-500" />

      <NuxtLink
          :to="{
          name: 'repo',
          params: { repo: repoName },
          query: currentDirectoryId ? { folder: currentDirectoryId } : {}
        }"
          class="font-semibold text-primary-500 hover:underline"
      >
        {{ repoName }}
      </NuxtLink>

      <span v-if="currentPath" class="text-zinc-400">/</span>
      <span v-if="currentPath" class="text-zinc-600 dark:text-zinc-300">{{ currentPath }}</span>
      <UBadge v-if="repo?.is_private" color="neutral" size="xs">Private</UBadge>
    </div>

    <!-- Controls -->
    <div class="mb-4 flex gap-2">
      <UButton v-if="currentPath" icon="i-heroicons-arrow-left" color="neutral" variant="ghost" size="sm" @click="goUp">Back</UButton>
      <USelectMenu :options="['master']" v-model="currentBranch" size="sm" />
    </div>

    <!-- VIEW MODE: TREE (File List) -->
    <div v-if="viewType === 'tree'" class="space-y-6">
      <UCard class="mb-8">
        <div class="border-b border-zinc-100 dark:border-zinc-800 p-3 bg-zinc-50 dark:bg-zinc-900/50 rounded-t-lg flex justify-between">
          <div class="flex items-center gap-2 text-sm text-zinc-500">
            <span class="font-mono">latest commit</span>
          </div>
        </div>

        <div v-if="loadingFiles" class="p-8 flex justify-center">
          <UIcon name="i-heroicons-arrow-path" class="animate-spin w-6 h-6 text-zinc-400" />
        </div>

        <div v-else class="divide-y divide-zinc-100 dark:divide-zinc-800">
          <div v-if="currentPath" @click="goUp" class="p-2 px-3 text-blue-500 hover:bg-zinc-50 dark:hover:bg-zinc-800/50 cursor-pointer text-sm font-bold">..</div>

          <div
              v-for="file in files"
              :key="file.path"
              @click="navigateToItem(file)"
              class="flex items-center justify-between p-3 hover:bg-zinc-50 dark:hover:bg-zinc-800/50 cursor-pointer group"
          >
            <div class="flex items-center gap-3">
              <UIcon :name="getFileIcon(file)" :class="file.is_directory ? 'text-blue-400' : 'text-zinc-400'" class="w-5 h-5" />
              <span class="text-sm text-zinc-700 dark:text-zinc-200 group-hover:text-primary-500 group-hover:underline">{{ file.name }}</span>
            </div>
            <span class="text-xs text-zinc-400">{{ file.size > 0 ? file.size + ' B' : '' }}</span>
          </div>

          <div v-if="!files || files.length === 0" class="p-8 text-center text-zinc-400 text-sm">No files found.</div>
        </div>
      </UCard>

      <!-- README PREVIEW -->
      <UCard v-if="readmeContent" class="overflow-hidden">
        <template #header>
          <div class="flex items-center gap-2 font-bold text-sm">
            <UIcon name="i-heroicons-document-text" />
            README.md
          </div>
        </template>
        <div class="p-6 prose dark:prose-invert max-w-none text-sm whitespace-pre-wrap font-sans">
          {{ readmeContent.content }}
        </div>
      </UCard>
    </div>

    <!-- VIEW MODE: BLOB (File Content) -->
    <UCard v-else-if="viewType === 'blob'" class="overflow-hidden">
      <div class="border-b border-zinc-200 dark:border-zinc-800 p-2 bg-zinc-100 dark:bg-zinc-900 flex justify-between items-center">
        <div class="text-sm font-mono text-zinc-600 dark:text-zinc-300 px-2">{{ currentPath }}</div>
        <UButton icon="i-heroicons-clipboard" color="neutral" variant="ghost" size="xs">Copy</UButton>
      </div>

      <div v-if="loadingBlob" class="p-12 flex justify-center">
        <UIcon name="i-heroicons-arrow-path" class="animate-spin w-8 h-8 text-zinc-300" />
      </div>

      <div v-else class="p-0 overflow-x-auto">
        <pre class="text-sm font-mono p-4 text-zinc-800 dark:text-zinc-200">{{ fileContent?.content }}</pre>
      </div>
    </UCard>
  </div>
</template>