#include "fs_commit.h"

const int FSCommit::Type = 0xf3123ae;

FSCommit::FSCommit(GitCommit && commit) : FSTree(commit.tree()), mCommit(std::move(commit))
{
}

FSCommit::~FSCommit()
{
}

int FSCommit::type() const
{
	return Type;
}
