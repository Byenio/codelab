<script setup lang="ts">
import { computed } from 'vue'
import { useApi } from '~/composables/useApi'

const props = defineProps<{
  repoId: number
  prId: number
}>()

defineEmits(['back'])

const { data, pending, error } = useApi<{ diff: string }>(
    `/api/v1/repositories/${props.repoId}/pull-requests/${props.prId}/diff`
)

interface DiffLine {
  text: string
  type: 'addition' | 'deletion' | 'chunk-header' | 'normal'
}

interface ParsedFile {
  heading: string
  lines: DiffLine[]
}

// Client-side unified diff text parser
const parsedFiles = computed<ParsedFile[]>(() => {
  if (!data.value?.diff) return []

  const files: ParsedFile[] = []
  const rawLines = data.value.diff.split('\n')
  let currentFile: ParsedFile | null = null

  for (const line of rawLines) {
    // Detect file header start
    if (line.startsWith('diff --git')) {
      const match = line.match(/b\/(.+)$/)
      const filename = match ? match[1] : 'Modified File'

      currentFile = { heading: filename, lines: [] }
      files.push(currentFile)
      continue
    }

    if (!currentFile) continue

    // Omit descriptive metadata lines to keep the UI clean
    if (line.startsWith('index ') || line.startsWith('--- ') || line.startsWith('+++ ')) {
      continue
    }

    // Classify line decorations
    if (line.startsWith('+')) {
      currentFile.lines.push({ text: line, type: 'addition' })
    } else if (line.startsWith('-')) {
      currentFile.lines.push({ text: line, type: 'deletion' })
    } else if (line.startsWith('@@')) {
      currentFile.lines.push({ text: line, type: 'chunk-header' })
    } else {
      currentFile.lines.push({ text: line, type: 'normal' })
    }
  }

  return files
})
</script>

<template>
  <div class="space-y-4">
    <div class="flex items-center gap-2">
      <UButton
          icon="i-heroicons-arrow-left"
          color="neutral"
          variant="ghost"
          size="sm"
          @click="$emit('back')"
      >
        Back to PRs
      </UButton>
      <h4 class="font-semibold text-sm">Changes for PR #{{ prId }}</h4>
    </div>

    <div v-if="pending" class="p-8 flex justify-center">
      <UIcon name="i-heroicons-arrow-path" class="animate-spin w-6 h-6 text-zinc-400" />
    </div>

    <div v-else-if="error" class="p-4 text-sm text-center text-error-500 bg-error-50 dark:bg-error-950/20 rounded-md">
      Failed to load file changes.
    </div>

    <div v-else-if="!parsedFiles.length" class="p-8 text-center text-sm">
      <div v-if="data?.diff && data.diff.trim()" class="text-left max-w-3xl mx-auto bg-red-50 dark:bg-red-950/30 border border-red-200 dark:border-red-900/50 p-4 rounded-lg font-mono text-red-600 dark:text-red-400 whitespace-pre-wrap">
        <div class="font-bold mb-2 flex items-center gap-2 text-xs uppercase tracking-wider text-red-700 dark:text-red-300">
          <UIcon name="i-heroicons-exclamation-triangle" class="w-4 h-4" />
          Git Execution / Repository Error
        </div>
        {{ data.diff }}
      </div>
      <span v-else class="text-zinc-500">No differences found in this Pull Request.</span>
    </div>

    <div v-else class="space-y-6">
      <div
          v-for="file in parsedFiles"
          :key="file.heading"
          class="border border-zinc-200 dark:border-zinc-800 rounded-lg overflow-hidden bg-white dark:bg-zinc-900"
      >
        <div class="bg-zinc-50 dark:bg-zinc-800/50 px-4 py-2 border-b border-zinc-200 dark:border-zinc-800 flex items-center gap-2 text-xs font-mono text-zinc-700 dark:text-zinc-300">
          <UIcon name="i-heroicons-document-text" class="w-4 h-4 text-zinc-400" />
          <span>{{ file.heading }}</span>
        </div>

        <div class="overflow-x-auto font-mono text-xs leading-relaxed p-2 bg-zinc-50 dark:bg-zinc-950">
          <pre class="min-w-max"><template v-for="(line, idx) in file.lines" :key="idx"><div
              :class="[
    'px-4 py-0.5 whitespace-pre font-mono',
    line.type === 'addition' ? 'bg-green-500/10 text-green-700 dark:text-green-400' : '',
    line.type === 'deletion' ? 'bg-red-500/10 text-red-700 dark:text-red-400' : '',
    line.type === 'chunk-header' ? 'bg-blue-500/5 text-zinc-400 font-bold border-y border-zinc-200/20 my-1' : '',
    line.type === 'normal' ? 'text-zinc-600 dark:text-zinc-400' : ''
  ]"
          >{{ line.text }}</div></template></pre>
        </div>
      </div>
    </div>
  </div>
</template>