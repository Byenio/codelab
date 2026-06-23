export interface User {
  id: number
  username: string
  email?: string
}

export interface Repository {
  id: number
  user_id: number
  directory_id?: number
  name: string
  description: string
  is_private: boolean
  created_at: string
  updated_at?: string
}

export interface Directory {
  id: number
  parent_id?: number
  name: string
}

export interface FileEntry {
  name: string
  path: string
  is_directory: boolean
  last_commit_msg: string
  last_commit_date: string
  size: number
}

export interface SSHKey {
  id: number
  title: string
  key: string
  created_at: string
  fingerprint?: string
}

export interface Branch {
  name: string
  sha: string
  is_head: boolean
}

export interface Collaborator {
  id: number
  username: string
}

export interface PullRequest {
  id: number
  repository_id: number
  author_id: number
  title: string
  description: string
  source_branch: string
  target_branch: string
  status: 'open' | 'merged' | 'closed'
  created_at: string
}
