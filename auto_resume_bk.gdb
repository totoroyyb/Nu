break BackEndService::ReadPosts
commands
silent
print "bk hit ReadPosts"
ignore 1 1000
continue
end

break BackEndService::RemovePosts
commands
silent
print "bk hit WriteUserTimeline"
ignore 2 1000
continue
end
