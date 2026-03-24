<script setup lang="ts">
import { useApi } from '~/composables/useApi'

const props = defineProps<{
  repo: {
    id: number
    name: string
    is_private: boolean
    description?: string
  }
  directoryId?: number
}>()

const route = useRoute()
const router = useRouter()

// --- State from Query Params ---
const currentBranch = computed({
  get: () => (route.query.branch as string) || 'master',
  set: val => router.replace({ query: { ...route.query, branch: val } })
})

const currentPath = computed(() => (route.query.path as string) || '')
const viewType = computed(() => (route.query.type as string) || 'tree')

// --- API URLs ---
const apiQuery = computed(() => {
  const q: any = { branch: currentBranch.value, path: currentPath.value }
  if (props.directoryId) q.directory_id = props.directoryId
  return q
})

// 1. Tree
const treeUrl = computed(() => `/api/v1/repositories/${props.repo.name}/tree`)
const { data: files, pending: loadingFiles } = useApi<any[]>(treeUrl.value, {
  query: apiQuery,
  watch: [treeUrl, currentBranch, currentPath],
  immediate: viewType.value === 'tree'
})

// 2. Blob
const blobUrl = computed(() => `/api/v1/repositories/${props.repo.name}/blob`)
const { data: fileContent, pending: loadingBlob } = useApi<{ content: string }>(blobUrl.value, {
  query: apiQuery,
  watch: [blobUrl, currentBranch, currentPath],
  immediate: viewType.value === 'blob'
})

// 3. Readme
const hasReadme = computed(() => {
  if (!files.value) return null
  return files.value.find((f: any) => f.name.toLowerCase() === 'readme.md')
})

const readmeUrl = computed(() => {
  if (viewType.value !== 'tree' || !hasReadme.value) return null
  return `/api/v1/repositories/${props.repo.name}/blob`
})

const readmeQuery = computed(() => {
  if (!hasReadme.value) return {}
  const path = currentPath.value ? `${currentPath.value}/${hasReadme.value.name}` : hasReadme.value.name
  const q = { ...apiQuery.value, path }
  return q
})

const { data: readmeContent } = useApi<{ content: string }>(
  computed(() => hasReadme.value ? `/api/v1/repositories/${props.repo.name}/blob` : null),
  {
    query: computed(() => ({
      path: hasReadme.value?.name,
      branch: 'master',
      directory_id: props.directoryId
    }))
  }
)

// --- Actions ---

const navigateToItem = (item: any) => {
  // Fix for potential null/undefined item names
  if (!item || !item.name) return

  const newPath = currentPath.value ? `${currentPath.value}/${item.name}` : item.name
  const newType = item.is_directory ? 'tree' : 'blob'

  router.push({
    query: {
      ...route.query,
      path: newPath,
      type: newType
    }
  })
}

const goUp = () => {
  if (!currentPath.value) return
  const segments = currentPath.value.split('/')
  segments.pop()
  const newPath = segments.join('/')

  router.push({
    query: {
      ...route.query,
      path: newPath || undefined,
      type: 'tree'
    }
  })
}

const iconMap: Record<string, string> = {
  'README.md': 'i-heroicons-document-text',
  'default': 'i-heroicons-document',
  'folder': 'i-heroicons-folder'
}
const getFileIcon = (file: any) => {
  if (file.is_directory) return iconMap['folder']
  return iconMap[file.name] || iconMap['default']
}
</script>

<template>
  <div>
    <!-- Header -->
    <div class="flex items-center gap-2 mb-6">
      <UIcon
        name="i-heroicons-book-open"
        class="w-6 h-6 text-zinc-400"
      />
      <h1 class="text-2xl font-bold text-zinc-900 dark:text-white">
        {{ repo.name }}
      </h1>
      <UBadge
        v-if="repo.is_private"
        color="neutral"
      >
        Private
      </UBadge>
    </div>

    <!-- Controls -->
    <div class="mb-4 flex gap-2">
      <UButton
        v-if="currentPath"
        icon="i-heroicons-arrow-left"
        color="neutral"
        variant="ghost"
        size="sm"
        @click="goUp"
      >
        Back
      </UButton>
      <USelectMenu
        v-model="currentBranch"
        :options="['master']"
        size="sm"
      />
      <div class="ml-auto font-mono text-sm text-zinc-400 flex items-center">
        {{ currentPath || '/' }}
      </div>
    </div>

    <!-- VIEW MODE: TREE -->
    <div
      v-if="viewType === 'tree'"
      class="space-y-6"
    >
      <UCard class="p-0 overflow-hidden">
        <div class="border-b border-zinc-100 dark:border-zinc-800 p-3 bg-zinc-50 dark:bg-zinc-900/50 flex justify-between">
          <div class="text-sm text-zinc-500 font-mono">
            latest commit
          </div>
        </div>

        <div
          v-if="loadingFiles"
          class="p-8 flex justify-center"
        >
          <UIcon
            name="i-heroicons-arrow-path"
            class="animate-spin w-6 h-6 text-zinc-400"
          />
        </div>

        <div
          v-else
          class="divide-y divide-zinc-100 dark:divide-zinc-800"
        >
          <div
            v-if="currentPath"
            class="p-2 px-3 text-blue-500 hover:bg-zinc-50 dark:hover:bg-zinc-800/50 cursor-pointer text-sm font-bold"
            @click="goUp"
          >
            ..
          </div>

          <div
            v-for="file in files"
            :key="file.path"
            class="flex items-center justify-between p-3 px-4 hover:bg-zinc-50 dark:hover:bg-zinc-800/50 cursor-pointer group"
            @click="navigateToItem(file)"
          >
            <div class="flex items-center gap-3">
              <UIcon
                :name="getFileIcon(file)"
                :class="file.is_directory ? 'text-blue-400' : 'text-zinc-400'"
                class="w-5 h-5"
              />
              <span class="text-sm text-zinc-700 dark:text-zinc-200 group-hover:text-primary-500 hover:underline">
                {{ file.name }}
              </span>
            </div>
            <span class="text-xs text-zinc-400 font-mono">{{ file.size > 0 ? file.size + ' B' : '' }}</span>
          </div>

          <div
            v-if="!files || files.length === 0"
            class="p-8 text-center text-zinc-400 text-sm"
          >
            Empty repository.
          </div>
        </div>
      </UCard>

      <!-- README PREVIEW -->
      <UCard
        v-if="readmeContent"
        class="overflow-hidden"
      >
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

    <!-- VIEW MODE: BLOB -->
    <UCard
      v-else-if="viewType === 'blob'"
      class="overflow-hidden p-0"
    >
      <div class="border-b border-zinc-200 dark:border-zinc-800 p-2 bg-zinc-100 dark:bg-zinc-900 flex justify-between items-center">
        <div class="text-sm font-mono text-zinc-600 dark:text-zinc-300 px-2">
          {{ currentPath }}
        </div>
        <UButton
          icon="i-heroicons-clipboard"
          color="neutral"
          variant="ghost"
          size="xs"
        >
          Copy
        </UButton>
      </div>

      <div
        v-if="loadingBlob"
        class="p-12 flex justify-center"
      >
        <UIcon
          name="i-heroicons-arrow-path"
          class="animate-spin w-8 h-8 text-zinc-300"
        />
      </div>

      <div
        v-else
        class="overflow-x-auto"
      >
        <pre class="text-sm font-mono p-4 text-zinc-800 dark:text-zinc-200">{{ fileContent?.content }}</pre>
      </div>
    </UCard>
  </div>
</template>
