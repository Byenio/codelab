<script setup lang="ts">
import { useApi } from '~/composables/useApi'
import authenticated from '~/middleware/authenticated'
import RepositoryView from '~/components/RepositoryView.vue'

definePageMeta({
  middleware: authenticated
})

const { user } = useAuth()
const route = useRoute()
const router = useRouter()
const toast = useToast()

const username = computed(() => route.params.username as string)

const currentPath = computed(() => {
  const slug = route.params.slug
  if (!slug || slug === 'null' || slug === 'undefined') return ''
  const joined = Array.isArray(slug) ? slug.join('/') : String(slug)
  return (joined === 'null' || joined === 'undefined') ? '' : joined
})

const { data, error, refresh } = await useApi<any>('/api/v1/fs/resolve', {
  query: computed(() => ({
    username: username.value,
    path: currentPath.value
  })),
  watch: [currentPath]
})

const isCreateFolderOpen = ref(false)
const newFolderName = ref('')
const isCreating = ref(false)

watch(newFolderName, (newValue) => {
  const formatted = newValue.replace(/\s+/g, '-')
  if (newValue !== formatted) {
    newFolderName.value = formatted
  }
})

const currentFolderId = computed(() => data.value?.directory_id || null)

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
    if (refresh) await refresh()
    toast.add({ title: 'Success', description: 'Folder created', color: 'success' })
  } catch (e) {
    toast.add({ title: 'Error', description: 'Failed to create folder', color: 'error' })
  } finally {
    isCreating.value = false
  }
}

const isDirectory = computed(() => !data.value || data.value.type === 'directory')
const isRepository = computed(() => data.value && data.value.type === 'repository')
const isFile = computed(() => data.value && data.value.type === 'file')

const repoData = computed(() => data.value?.repository)
const repoDirId = computed(() => data.value?.directory_id)

const slugify = (text: string) => {
  return text
    .toString()
    .toLowerCase()
    .replace(/\s+/g, '-')
    .replace(/[^\w\-]+/g, '')
    .replace(/\-\-+/g, '-')
    .replace(/^-+/, '')
    .replace(/-+$/, '')
}

const navigateToItem = (item: any) => {
  const slugName = slugify(item.name)
  const newPath = currentPath.value ? `${currentPath.value}/${slugName}` : slugName
  router.push(`/u/${username.value}/${newPath}`)
}

const breadcrumbs = computed(() => {
  const crumbs = [{ name: username.value, path: `/u/${username.value}`}]
  if (!currentPath.value) return crumbs

  const segments = currentPath.value.split('/')
  let accum = ''
  for (const seg of segments) {
    accum = accum ? `${accum}/${seg}` : seg
    crumbs.push({
      name: seg,
      path: `/u/${username.value}/${accum}`
    })
  }
  return crumbs
})
</script>

<template>
  <div class="min-h-screen bg-zinc-50 dark:bg-zinc-950">
    <main class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-8">
      <!-- Breadcrumbs Header -->
      <div class="flex items-center gap-2 text-sm text-zinc-500 mb-6">
        <NuxtLink
          :to="username ? `/u/${username}` : `/u/${user.value.username}`"
          class="hover:text-primary-500 flex items-center gap-1"
        >
          <UIcon
            name="i-heroicons-folder"
            class="w-4 h-4"
          />
        </NuxtLink>
        <template
          v-for="crumb in breadcrumbs"
          :key="crumb.path"
        >
          <span class="text-zinc-300">/</span>
          <NuxtLink
            :to="crumb.path"
            class="hover:text-primary-500 font-medium text-zinc-700 dark:text-zinc-200"
          >
            {{ crumb.name }}
          </NuxtLink>
        </template>
      </div>

      <!-- ERROR STATE -->
      <div
        v-if="error"
        class="text-center py-12"
      >
        <UIcon
          name="i-heroicons-exclamation-triangle"
          class="w-12 h-12 text-zinc-300 mx-auto mb-4"
        />
        <h3 class="text-lg font-medium text-zinc-900 dark:text-white">
          Path not found
        </h3>
        <p class="text-zinc-500 mt-2">
          The directory or repository you are looking for does not exist.
        </p>
        <UButton
          to="/"
          variant="ghost"
          class="mt-4"
        >
          Go Home
        </UButton>
      </div>

      <!-- DIRECTORY VIEW -->
      <div v-else-if="isDirectory && data">
        <div class="flex justify-between items-center mb-6">
          <h1 class="text-2xl font-bold text-zinc-900 dark:text-white">
            {{ breadcrumbs.length ? breadcrumbs[breadcrumbs.length-1].name : 'Home' }}
          </h1>
          <div class="flex gap-2">
            <UButton
              icon="i-heroicons-folder-plus"
              color="neutral"
              label="New Folder"
              @click="isCreateFolderOpen = true"
            />

            <UButton
              :to="currentFolderId ? `/new?folder=${currentFolderId}&path=${currentPath}` : `/new?path=${currentPath}`"
              icon="i-heroicons-plus"
              color="primary"
            >
              New Repository
            </UButton>
          </div>
        </div>

        <UCard class="p-0 overflow-hidden">
          <div class="divide-y divide-zinc-100 dark:divide-zinc-800">
            <div
              v-if="data.directories.length === 0 && data.repositories.length === 0"
              class="p-8 text-center text-zinc-500"
            >
              Empty directory.
            </div>

            <!-- Directories -->
            <div
              v-for="dir in data.directories"
              :key="'d'+dir.id"
              class="flex items-center gap-3 p-3 px-4 hover:bg-zinc-50 dark:hover:bg-zinc-800/50 cursor-pointer group"
              @click="navigateToItem(dir)"
            >
              <UIcon
                name="i-heroicons-folder"
                class="w-5 h-5 text-primary-400"
              />
              <span class="text-sm font-medium text-zinc-700 dark:text-zinc-200 group-hover:text-primary-500">{{ dir.name }}</span>
            </div>

            <!-- Repositories -->
            <div
              v-for="repo in data.repositories"
              :key="'r'+repo.id"
              class="flex items-center gap-3 p-3 px-4 hover:bg-zinc-50 dark:hover:bg-zinc-800/50 cursor-pointer group"
              @click="navigateToItem(repo)"
            >
              <UIcon
                name="i-heroicons-book-open"
                class="w-5 h-5 text-zinc-400"
              />
              <span class="text-sm font-bold text-zinc-700 dark:text-zinc-200 group-hover:text-primary-500">{{ repo.name }}</span>
              <UIcon
                v-if="repo.is_private"
                name="i-heroicons-lock-closed"
                class="w-3 h-3 text-zinc-400"
              />
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

      <!-- Modal -->
      <UModal
        v-model:open="isCreateFolderOpen"
        title="Create New Folder"
      >
        <template #body>
          <form
            class="space-y-4"
            @submit.prevent="createFolder"
          >
            <UFormField label="Folder Name">
              <UInput
                v-model="newFolderName"
                placeholder="e.g. My Projects"
                autofocus
              />
            </UFormField>

            <div class="flex justify-end gap-2 pt-2">
              <UButton
                color="neutral"
                variant="ghost"
                @click="isCreateFolderOpen = false"
              >
                Cancel
              </UButton>
              <UButton
                type="submit"
                :loading="isCreating"
                :disabled="!newFolderName"
              >
                Create
              </UButton>
            </div>
          </form>
        </template>
      </UModal>
    </main>
  </div>
</template>
