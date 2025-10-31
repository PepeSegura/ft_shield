#include "ft_shield.h"

t_list	*ft_lstnew(void *content)
{
	t_list *lst = calloc(sizeof(t_list), 1);

	if (lst == NULL)
		return (NULL);
	lst->content = content;
	return (lst);
}

t_list	*ft_lstlast(t_list *lst)
{
	t_list *aux = lst;

	if (aux == NULL)
		return (NULL);
	while (aux->next != NULL)
		aux = aux->next;
	return (aux);
}

void	ft_lstadd_back(t_list **lst, t_list *new_node)
{
	if (new_node == NULL)
		return ;
	if (*lst == NULL)
	{
		*lst = new_node;
		return ;
	}
	ft_lstlast(*lst)->next = new_node;
}

bool shell_was_closed(t_list *node)
{
	pid_t pid_shell = (int)(long)node->content;

	int status;
	int res_wait = waitpid(pid_shell, &status, WNOHANG);
	if (pid_shell)
		ft_dprintf(2, "active shell: %d res wait %d\n", pid_shell, res_wait);
	if (res_wait == pid_shell)
	{
		ft_dprintf(2, "process %d has finished\n", res_wait);
		return (true);
	}
	return (false);
}

void ft_delete_node_if_true(t_server *server, t_list **lst, bool (*f)(t_list *))
{
	t_list *current = *lst;
	t_list *prev = NULL;
	t_list *next;

	while (current != NULL)
	{
		next = current->next;
		if (f(current) == true)
		{
			ft_dprintf(2, "deleting node with content %ld\n", (long)current->content);
			server->nbr_clients--;
			ft_dprintf(2, "there are %d connections\n", server->nbr_clients);
			if (prev == NULL)
				*lst = next;
			else
				prev->next = next;
			free(current);
		}
		else
			prev = current;
		current = next;
	}
}

void	ft_lstdelone(t_list *lst, void (*del)(void *))
{
	if (del != NULL)
		(*del)(lst->content);
	free(lst);
}

void	ft_lstclear(t_list **lst, void (*del)(void *))
{
	t_list	*tmp;

	while (lst && *lst)
	{
		tmp = (*lst)->next;
		ft_lstdelone(*lst, del);
		*lst = tmp;
	}
}
