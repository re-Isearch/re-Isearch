import sys
import huggingface_hub as hh

if __name__ == '__main__':
    # get command line args
    if len(sys.argv) != 2:
        print('Usage: python download-repo.py <repo_id>')
        sys.exit(1)
    repo_id = sys.argv[1]

    # check valid repo
    if '/' not in repo_id:
        print('repo_id should be of the form <user>/<repo>')
    _, model_name = repo_id.split('/')

    # download model
    hh.snapshot_download(
        repo_id=repo_id, local_dir=f'./{model_name}', local_dir_use_symlinks=False
    )
    print(f'{repo_id} downloaded successfully')
