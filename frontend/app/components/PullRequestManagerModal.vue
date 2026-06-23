<script setup lang="ts">
import { ref, computed } from 'vue'
import { useApi } from '~/composables/useApi'
import type { PullRequest, Branch } from '~/types/models'

const props = defineProps<{
  repoId: number
}>()

const isOpen = defineModel<boolean>()

const toast = useToast()

const { data: prs, refresh } = useApi<PullRequest[]>(`/api/v1/repositories/${props.repoId}/pull-requests`)
const { data: branches } = useApi<Branch[]>(`/api/v1/repositories/${props.repoId}/branches`)

const newTitle = ref('')
const newDescription = ref('')
const sourceBranch = ref('')
const targetBranch = ref('master')

const createPR = async () => {
  if (!newTitle.value || !sourceBranch.value || !targetBranch.value) return
  try {
    await useApi(`/api/v1/repositories/${props.repoId}/pull-requests`, {
      method: 'POST',
      body: {
        title: newTitle.value,
        description: newDescription.value,
        source_branch: sourceBranch.value,
        target_branch: targetBranch.value
      }
    })
    newTitle.value = ''
    newDescription.value = ''
    sourceBranch.value = ''
    await refresh()
    toast.add({ title: 'Pull request created', color: 'success' })
  } catch (e) {
    toast.add({ title: 'Error', description: 'Failed to create pull request', color: 'error' })
  }
}

const mergePR = async (id: number) => {
  try {
    await useApi(`/api/v1/repositories/${props.repoId}/pull-requests/${id}/merge`, { method: 'POST' })
    await refresh()
    toast.add({ title: 'Pull request merged', color: 'success' })
  } catch (e) {
    toast.add({ title: 'Error', description: 'Failed to merge pull request (conflicts?)', color: 'error' })
  }
}

const closePR = async (id: number) => {
  try {
    await useApi(`/api/v1/repositories/${props.repoId}/pull-requests/${id}/close`, { method: 'POST' })
    await refresh()
    toast.add({ title: 'Pull request closed', color: 'success' })
  } catch (e) {
    toast.add({ title: 'Error', description: 'Failed to close pull request', color: 'error' })
  }
}
</script>

<template>
  <UModal v-model:open="isOpen" title="Pull Requests">
    <template #body>
      <div class="space-y-6 max-h-[70vh] overflow-y-auto">
        <!-- List PRs -->
        <div>
          <h4 class="font-medium text-sm mb-2">Existing Pull Requests</h4>
          <ul v-if="prs?.length" class="divide-y divide-zinc-200 dark:divide-zinc-800 border dark:border-zinc-800 rounded-md">
            <li v-for="pr in prs" :key="pr.id" class="p-3 text-sm flex flex-col gap-2">
              <div class="flex justify-between items-start">
                <div>
                  <span class="font-bold">#{{ pr.id }} {{ pr.title }}</span>
                  <div class="text-xs text-zinc-500 mt-1">
                    {{ pr.source_branch }} &rarr; {{ pr.target_branch }}
                  </div>
                </div>
                <UBadge :color="pr.status === 'open' ? 'green' : pr.status === 'merged' ? 'purple' : 'red'">
                  {{ pr.status }}
                </UBadge>
              </div>
              <div v-if="pr.status === 'open'" class="flex gap-2 justify-end mt-2">
                <UButton color="green" size="xs" icon="i-heroicons-check" @click="mergePR(pr.id)">Merge</UButton>
                <UButton color="red" size="xs" variant="ghost" icon="i-heroicons-x-mark" @click="closePR(pr.id)">Close</UButton>
              </div>
            </li>
          </ul>
          <p v-else class="text-sm text-zinc-500">No pull requests found.</p>
        </div>

        <!-- Create PR -->
        <div class="pt-4 border-t dark:border-zinc-800">
          <h4 class="font-medium text-sm mb-2">Open New PR</h4>
          <div class="flex flex-col gap-3">
            <div class="flex gap-2">
              <USelectMenu v-model="sourceBranch" :items="branches?.map(b => b.name) || []" placeholder="Source" class="flex-1" />
              <span class="text-zinc-500 self-center">into</span>
              <USelectMenu v-model="targetBranch" :items="branches?.map(b => b.name) || []" placeholder="Target" class="flex-1" />
            </div>
            <UInput v-model="newTitle" placeholder="PR Title" />
            <UTextarea v-model="newDescription" placeholder="Description (optional)" />
            <UButton @click="createPR" class="self-start" icon="i-heroicons-plus">Create Pull Request</UButton>
          </div>
        </div>
      </div>
    </template>
  </UModal>
</template>
